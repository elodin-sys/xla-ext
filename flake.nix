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
        isLinux = pkgs.stdenv.isLinux;

        sdkroot = "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk";

        # XLA pin
        XLA_REV = "98bc2a2e3a87ef3e4b41a59a251462089bfc998d";

        xlaSrc = pkgs.fetchFromGitHub {
          owner = "openxla";
          repo = "xla";
          rev = XLA_REV;
          sha256 = "sha256-sey2yXF3tofTgmS1wXJZS6HwngzBYzktl/QRbMZfrYE=";
        };

        python = pkgs.python311;
        pythonPkgs = python.pkgs;

        # Pre-fetched Bazel externals
        rules_python = pkgs.fetchzip {
          url = "https://github.com/bazelbuild/rules_python/releases/download/0.39.0/rules_python-0.39.0.tar.gz";
          sha256 = "sha256-UdLXEf/9POmYSoLzkHY9PjQFqLO/PMSiV3GE+CoQPOM=";
        };

        bazelPrebuilt = pkgs.stdenv.mkDerivation {
          pname = "bazel-prebuilt";
          version = "7.6.0";
          buildInputs = lib.optionals isLinux [pkgs.stdenv.cc.cc.lib];
          propagatedBuildInputs = lib.optionals isLinux [pkgs.stdenv.cc.cc.lib];
          src = with pkgs;
            fetchurl (
              if system == "aarch64-darwin"
              then {
                url = "https://releases.bazel.build/7.6.0/release/bazel-7.6.0-darwin-arm64";
                sha256 = "sha256-4bRp4OvkRIvhpZ2r/eFJdwrByECHy3rncDEM1tClFYo=";
              }
              else if system == "aarch64-linux"
              then {
                url = "https://github.com/bazelbuild/bazel/releases/download/7.6.0/bazel-7.6.0-linux-arm64";
                sha256 = "sha256-4iqN5wFYUZPYhqKaytll9wcNtamLRNL8IvxE5l2p57U=";
              }
              else if system == "x86_64-linux"
              then {
                url = "https://github.com/bazelbuild/bazel/releases/download/7.6.0/bazel-7.6.0-linux-x86_64";
                sha256 = "sha256-0NNmgQjDle7ia9O9LSUShf3W3N1w8itskUXnljrajmM=";
              }
              else builtins.throw "Unsupported system ${system}"
            );
          dontUnpack = true;
          phases = ["installPhase"];
          installPhase = ''
            mkdir -p $out/bin
            install -Dm755 "$src" "$out/bin/bazel"
          '';
        };

        bazel = if isDarwin then
          bazelPrebuilt
          // {
            override = _args: bazelPrebuilt;
          }
        else pkgs.bazel_7;
        bazelBin = "${bazel}/bin/bazel";

        llvm = pkgs.llvmPackages_latest;
        
        commonNativeBuildInputs =
          [
            bazel
            pkgs.git
            pkgs.unzip
            pkgs.wget
            python
            pythonPkgs.pip
            pythonPkgs.numpy
          ]
          ++ lib.optionals (!isDarwin) [
            llvm.clang
            pkgs.curl
            pkgs.binutils
            pkgs.libtool
          ];

        # tool paths
        darwinPath = "${python}/bin";
        linuxPath = "${pkgs.binutils}/bin:${pkgs.llvm}/bin:${python}/bin";
        gnuLibtool = "${pkgs.libtool}/bin/libtool";
        darwinDevDir = "/Applications/Xcode.app/Contents/Developer";

        # PATH for Bazel actions on macOS (shim first, then toolchains)
        actionEnvPath = lib.makeSearchPath "bin" [
          pkgs.coreutils
          pkgs.findutils
          pkgs.gnugrep
          pkgs.gnused
          pkgs.gawk
          pkgs.gnutar
          pkgs.gzip
          pkgs.zip
          pkgs.unzip
          pkgs.llvm
          python
        ];

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
            export DEVELOPER_DIR=${darwinDevDir}
            TOOLCHAIN=$DEVELOPER_DIR/Toolchains/XcodeDefault.xctoolchain
            export PATH=${darwinPath}:$TOOLCHAIN/usr/bin:$PATH
            export SDKROOT=${sdkroot}
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
            "--verbose_failures"
            (
              if isDarwin
              then "--config=macos"
              else "--config=linux"
            )
          ]
          # We still inject a PATH value for actions on macOS because it is built from Nix store paths.
          ++ lib.optionals isDarwin [
            "--repo_env=USE_HERMETIC_CC_TOOLCHAIN=0"
            "--action_env=PATH=${actionEnvPath}:/usr/bin"
            "--repo_env=PATH=${actionEnvPath}:/usr/bin"
            # target sysroot
            "--copt=-isysroot${sdkroot}"
            "--cxxopt=-isysroot${sdkroot}"
            "--conlyopt=-isysroot${sdkroot}"
            "--linkopt=-isysroot${sdkroot}"
            # host/exec sysroot
            "--host_copt=-isysroot${sdkroot}"
            "--host_cxxopt=-isysroot${sdkroot}"
            "--host_conlyopt=-isysroot${sdkroot}"
            "--host_linkopt=-isysroot${sdkroot}"
          ]
          ++ lib.optionals isLinux [
            "--override_repositoy=rules_python=${rules_python}"
          ]
          ++ lib.optionals cudaSupport ["--config=cuda"]
        );

        # Prepare XLA, overlay extension, and inject a separate macOS rc file, imported by .bazelrc
        mkXlaSrcWithExtension = {backend}:
          pkgs.runCommand "xla-ext-${backend}" {} ''
            set -eo pipefail
            cp -R ${xlaSrc} "$out"

            # Replace extension with ours
            rm -rf "$out/xla/extension"
            mkdir -p "$out/xla"
            chmod -R u+w $out
            cp -R ${self}/extension "$out/xla/extension"

            # NOW make the whole tree writable (do this AFTER the copy from the store)
            chmod -R u+w "$out"

            patch -p1 -d "$out" "$out"/xla/extension/patches/*
            # Pin Bazel version developers see
            echo 7.6.0 > "$out/.bazelversion"

            # Ensure root .bazelrc imports our fragment (idempotent)
            if ! grep -q 'xla/extension/bazelrc.fragment' "$out/.bazelrc" 2>/dev/null; then
              echo 'import xla/extension/bazelrc.fragment' >> "$out/.bazelrc"
            fi          '';

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

          prePhase = mkPhaseEnv {inherit backend cudaSupport cudaPkgs hostGccOpt;};

          impureHostDeps = lib.optionals isDarwin [
            "/Applications/Xcode.app"
            "/Library/Developer/CommandLineTools"
            "/usr/bin/xcrun"
            "/usr/bin/xcodebuild"
            "/usr/include"
          ];

          patchThingsUp = ''
            bash ./xla/extension/patch-stuff-pre-build.sh ${bazelBin}
          '';
        in
          pkgs.buildBazelPackage {
            pname = name;
            version = XLA_REV;
            inherit bazel;
            src = srcPrepared;

            # "allowed-impure-host-deps" = impureHostDeps;

            bazelFlags = buildFlags;
            
            buildAttrs = {
              buildInputs = lib.optional isDarwin [pkgs.apple-sdk_15];
              nativeBuildInputs = commonNativeBuildInputs ++ cudaInputs;
              preConfigure = prePhase;

              # "allowed-impure-host-deps" = impureHostDeps;

              bazelTargets = ["//xla/extension:tarball"];
              dontStrip = true;
              dontAddBazelOpts = true;

              preBuild = patchThingsUp;

              installPhase = ''
                set -eo pipefail
                TARBALL="$(find $bazelOut -name xla_extension.tar.gz -type f | head -n1)"
                if [ -z "$TARBALL" ]; then
                  echo "Tarball not found: $TARBALL"; exit 1
                fi
                mkdir -p $out
                cp -v "$TARBALL" "$out/${outName}"
              '';
            };

            fetchAttrs = {
              nativeBuildInputs = commonNativeBuildInputs ++ cudaInputs;
              # preBuild =
              #   prePhase
              #   + patchThingsUp;

              # "allowed-impure-host-deps" = impureHostDeps;

              bazelTargets = ["//xla/extension:tarball"];
              bazelFlags = fetchFlags;
              bazelFetchFlags = fetchFlags;
              sha256 = "sha256-rSIkf5cffyw7qY9YTRc3R/PXlqZVOhQ4hqGmLWEqWQY=";
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
            nativeBuildInputs =
              commonNativeBuildInputs
              ++ lib.optionals isDarwin
              [
                pkgs.apple-sdk_15
              ];

            LC_ALL = "C.UTF-8";

            shellHook = ''
              export DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer
            '';
          };

          checks.toolchain-verification = let
            prepared = mkXlaSrcWithExtension {backend = "cpu";};
          in
            pkgs.runCommand "bazel-toolchain-verification"
            {nativeBuildInputs = [bazel pkgs.coreutils pkgs.gnugrep pkgs.python3];} ''
              set -eu
              BAZEL="${bazel}/bin/bazel"
              cp -r ${prepared} ./src
              chmod -R u+w ./src
              cd ./src

              echo "==> Verifying Bazel selects @local_config_apple_cc on macOS"
              bazel cquery --config=macos \
                'kind(cc_toolchain, deps(@bazel_tools//tools/cpp:current_cc_toolchain))' \
                --output=label | tee cc_toolchain.txt

              grep -q '@local_config_apple_cc' cc_toolchain.txt
              ! grep -q 'local_config_cc' cc_toolchain.txt

              mkdir -p tools/check
              cat > tools/check/BUILD <<'EOF'
              cc_binary(
                name = "tiny",
                srcs = ["tiny.cc"],
              )
              EOF
              echo 'int main(){return 0;}' > tools/check/tiny.cc

              bazel build --config=macos --nobuild -s //tools/check:tiny | tee build.log
              cp cc_toolchain.txt "$out"
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
