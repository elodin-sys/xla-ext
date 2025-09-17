set -euo pipefail

BAZEL_BIN="${1:?pass bazel path as first arg}"

# BAZEL_OUT="$(echo ''${NIX_BUILD_TOP}/output | sed -e 's,//,/,g')"
# BAZEL_USER_ROOT="$(echo ''${NIX_BUILD_TOP}/tmp | sed -e 's,//,/,g')"

$BAZEL_BIN \
  --output_base="$bazelOut" \
  --output_user_root="$bazelUserRoot" \
  fetch --config=macos @com_github_grpc_grpc//:all

EXTERNAL_DIR="${bazelOut}/external"

GRPC_DIR="${EXTERNAL_DIR}/com_github_grpc_grpc"
PATCH_FILE="xla/extension/pre-build-patches/000-grpc-disable-universal.patch"

echo "Patching up GRPC at $GRPC_DIR..."

if [[ -d "$GRPC_DIR" ]]; then
  if ! grep -q "Patched: do not construct a universal binary" "$GRPC_DIR/bazel/grpc_build_system.bzl" 2>/dev/null; then
    echo "[patch] Applying gRPC universal-binary disable patch…"
    patch -p1 -d "$GRPC_DIR" < "$PATCH_FILE"
  else
    echo "[patch] gRPC already patched; skipping."
  fi
else
  echo "[patch] Could not find $GRPC_DIR (yet). For diagnostics:"
  ls -1 "$EXTERNAL_DIR" || true
  exit 1
fi

