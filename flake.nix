{
  description = "OpenXLA extension builds as Nix derivations (Bazel)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-25.05";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachSystem [
      "x86_64-linux"
      "aarch64-linux"
      "aarch64-darwin"
    ] (
      system: let
        pkgs = import nixpkgs {
          inherit system;
          config = {
            allowUnfree = true;
            cudaSupport = true;
          };
        };
        lib = pkgs.lib;
        isDarwin = pkgs.stdenv.isDarwin;

        # XLA pin (same as before)
        XLA_REV = "2a6015f068e4285a69ca9a535af63173ba92995b";

        xlaSrc = pkgs.fetchFromGitHub {
          owner = "openxla";
          repo = "xla";
          rev = XLA_REV;
          sha256 = "sha256-sey2yXF3tofTgmS1wXJZS6HwngzBYzktl/QRbMZfrYE=";
        };

        python = pkgs.python311;
        pythonPkgs = python.pkgs;

        # Pick one Bazel release and fetch the upstream prebuilt binary.
        bazelPrebuilt =
          if isDarwin
          then
            pkgs.stdenv.mkDerivation {
              pname = "bazel-prebuilt";
              version = "7.6.0";
              src = pkgs.fetchurl {
                url = "https://releases.bazel.build/7.6.0/release/bazel-7.6.0-darwin-arm64";
                sha256 = "sha256-4bRp4OvkRIvhpZ2r/eFJdwrByECHy3rncDEM1tClFYo=";
              };
              dontUnpack = true;
              phases = ["installPhase" "patchPhase"];
              installPhase = ''
                mkdir -p $out/bin
                install -Dm755 "$src" "$out/bin/bazel"
              '';
            }
          else
            pkgs.stdenv.mkDerivation {
              pname = "bazel-prebuilt";
              version = "7.6.0";
              src = pkgs.fetchurl {
                url = "https://github.com/bazelbuild/bazel/releases/download/7.6.0/bazel-7.6.0-linux-x86_64";
                sha256 = "sha256-0NNmgQjDle7ia9O9LSUShf3W3N1w8itskUXnljrajmM=";
              };
              dontUnpack = true;
              installPhase = ''
                install -Dm755 "$src" "$out/bin/bazel"
              '';
            };

        # bazel = pkgs.bazel_7.override {enableNixHacks = true;};
        bazel =
          bazelPrebuilt
          // {
            override = _args: bazelPrebuilt;
          };

        commonNativeBuildInputs =
          [
            bazel
            pkgs.llvm_20
            pkgs.git
            pkgs.unzip
            pkgs.wget
            pkgs.curl
            python
            pythonPkgs.pip
            pythonPkgs.numpy
          ]
          ++ (
            if isDarwin
            then [pkgs.darwin.cctools]
            else [pkgs.binutils pkgs.libtool]
          );

        # On macOS we keep Apple cctools & LLVM early on PATH
        darwinPath = "${pkgs.darwin.cctools}/bin:${pkgs.llvm}/bin:${python}/bin";
        linuxPath = "${pkgs.binutils}/bin:${pkgs.llvm}/bin:${python}/bin";
        darwinLibtool = "${pkgs.darwin.cctools}/bin/libtool";
        gnuLibtool = "${pkgs.libtool}/bin/libtool";

        # Phase env shared by fetch/build; runs your configure.py
        mkPhaseEnv = {
          backend,
          cudaSupport ? false,
          cudaPkgs ? null,
          hostGccOpt ? null,
        }: ''
          set -eo pipefail
          export HOME=$PWD/.home
          mkdir -p "$HOME"
          export LC_ALL=C.UTF-8

          ${lib.optionalString isDarwin ''
            export PATH=${darwinPath}:$PATH
            export LIBTOOL=${darwinLibtool}
          ''}
          ${lib.optionalString (!isDarwin) ''
            export PATH=${linuxPath}:$PATH
            export LIBTOOL=${gnuLibtool}
          ''}

          # Python needed by configure.py (quiet if already ok)
          ${python}/bin/python -m pip install --upgrade pip numpy >/dev/null 2>&1 || true

          ${lib.optionalString cudaSupport ''
            export CUDA_HOME=${cudaPkgs.cudatoolkit}
            export CUDA_PATH=${cudaPkgs.cudatoolkit}
            export CUDNN_PATH=${cudaPkgs.cudnn}
            export LD_LIBRARY_PATH=${cudaPkgs.cudatoolkit}/lib:${cudaPkgs.cudatoolkit}/lib64:${cudaPkgs.cudnn}/lib:$LD_LIBRARY_PATH
          ''}

          ${lib.optionalString (cudaSupport && hostGccOpt != null) ''
            export CC=${cudaPkgs.hostGcc}/bin/gcc
            export CXX=${cudaPkgs.hostGcc}/bin/g++
          ''}

          ${python}/bin/python ./configure.py --backend=${backend}
        '';

        # Minimal Bazel flags: just select our in-repo profile
        mkBazelFlags = {cudaSupport ? false}: (
          [
            (
              if isDarwin
              then "--config=macos"
              else "--config=linux"
            )
          ]
          ++ lib.optionals cudaSupport ["--config=cuda"]
        );

        # Copy upstream XLA and overlay our extension directory + .bazelrc import
        mkXlaSrcWithExtension = {backend}:
          pkgs.runCommand "xla-ext-${backend}" {} ''
            set -eo pipefail
            cp -R ${xlaSrc} $out
            chmod -R u+w $out

            rm -rf "$out/xla/extension"
            mkdir -p "$out/xla"
            cp -R ${self}/extension "$out/xla/extension"

            # Pin Bazel version used by developers to 7.6 (builder still uses nixpkgs bazel)
            echo "7.6.0" > "$out/.bazelversion"

            # Ensure root .bazelrc imports our fragment (idempotent)
            if ! grep -q 'xla/extension/bazelrc.fragment' "$out/.bazelrc" 2>/dev/null; then
              echo 'import xla/extension/bazelrc.fragment' >> "$out/.bazelrc"
            fi
          '';

        mkXlaTarball = {
          name,
          backend,
          platform,
          extraBazelFlags ? [],
          cudaSupport ? false,
          cudaNameForArtifact ? null,
        }: let
          srcPrepared = mkXlaSrcWithExtension {inherit backend;};
          cudaPkgs = pkgs.cudaPackages;

          # hostGcc exists only on x86_64-linux; guard it
          hostGccOpt = lib.attrByPath ["hostGcc"] null cudaPkgs;

          cudaInputs =
            lib.optionals cudaSupport [
              cudaPkgs.cudatoolkit
              cudaPkgs.cudnn
              cudaPkgs.cuda_nvcc
            ]
            ++ lib.optional (hostGccOpt != null) hostGccOpt;

          buildFlags = mkBazelFlags {inherit cudaSupport;} ++ extraBazelFlags;
          fetchFlags = buildFlags;

          xlaTargetForName =
            if cudaSupport
            then
              (
                if cudaNameForArtifact == null
                then "cuda"
                else cudaNameForArtifact
              )
            else "cpu";
          outName = "xla_extension-${platform}-${xlaTargetForName}.tar.gz";

          prePhase = mkPhaseEnv {
            inherit backend cudaSupport cudaPkgs hostGccOpt;
          };
        in
          pkgs.buildBazelPackage {
            pname = name;
            version = XLA_REV;
            inherit bazel;
            src = srcPrepared;

            buildAttrs = {
              nativeBuildInputs = commonNativeBuildInputs ++ cudaInputs;
              preConfigure = prePhase;

              bazelTargets = ["//xla/extension:tarball"];
              bazelFlags = buildFlags;
              dontStrip = true;

              installPhase = ''
                set -eo pipefail
                TARBALL="$(find bazel-bin -name xla_extension.tar.gz -type f | head -n1)"
                if [ -z "$TARBALL" ]; then
                  echo "tarball not found under bazel-bin"; exit 1
                fi
                mkdir -p $out
                cp -v "$TARBALL" "$out/${outName}"
              '';
            };

            fetchAttrs = {
              nativeBuildInputs = commonNativeBuildInputs ++ cudaInputs;
              preBuild = prePhase;
              bazelTargets = ["//xla/extension:tarball"];
              bazelFlags = fetchFlags;
              bazelFetchFlags = fetchFlags;
              # Keep your known-good source fetch hash
              sha256 = "sha256-aS15t5wpTsaIVJHvDZXOkSlN1uTMzEkn+ya+FQCtYgI=";
            };
          };
      in
        {
          packages.xla-tarball-linux-cpu = mkXlaTarball {
            name = "xla-tarball-linux-cpu";
            backend = "cpu";
            platform = "x86_64-linux-gnu";
          };

          packages.xla-tarball-macos-arm64-cpu = mkXlaTarball {
            name = "xla-tarball-macos-arm64-cpu";
            backend = "cpu";
            platform = "aarch64-darwin";
          };

          packages.xla-tarball-linux-arm64-cpu = mkXlaTarball {
            name = "xla-tarball-linux-arm64-cpu";
            backend = "cpu";
            platform = "aarch64-linux-gnu";
          };

          devShells.default = pkgs.mkShell {
            nativeBuildInputs = commonNativeBuildInputs;
            LC_ALL = "C.UTF-8";
          };

          # Run with: `nix flake check`
          checks.toolchain-verification = let
            prepared = mkXlaSrcWithExtension {backend = "cpu";};
          in
            pkgs.runCommand "bazel-toolchain-verification"
            {
              nativeBuildInputs = [bazel pkgs.coreutils pkgs.gnugrep pkgs.python3];
            } ''
              set -eu
              BAZEL="${bazel}/bin/bazel"
              # Work inside the prepared XLA workspace that already imports our fragment.
              cp -r ${prepared} ./src
              chmod -R u+w ./src
              cd ./src

              echo "==> Verifying Bazel selects @local_config_apple_cc on macOS"
              # Print the actual cc_toolchain target Bazel resolved to
              bazel cquery --config=macos \
                'kind(cc_toolchain, deps(@bazel_tools//tools/cpp:current_cc_toolchain))' \
                --output=label | tee cc_toolchain.txt

              # Assert we picked the stub we ship (and not local_config_cc)
              grep -q '@local_config_apple_cc' cc_toolchain.txt
              ! grep -q 'local_config_cc' cc_toolchain.txt             #

              cp cc_toolchain.txt "$out"

              mkdir -p tools/check
              cat > tools/check/BUILD <<'EOF'
              cc_binary(
                name = "tiny",
                srcs = ["tiny.cc"],
              )
              EOF
              echo 'int main(){return 0;}' > tools/check/tiny.cc

              bazel build --config=macos --nobuild -s //tools/check:tiny | tee build.log
            '';
        }
        // lib.optionalAttrs (system == "x86_64-linux") {
          packages.xla-tarball-linux-cuda = mkXlaTarball {
            name = "xla-tarball-linux-cuda";
            backend = "cuda";
            platform = "x86_64-linux-gnu";
            cudaSupport = true;
            cudaNameForArtifact = "cuda130";
          };
        }
    );
}
