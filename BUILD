cmd = """(
    cd /private/var/tmp/_bazel_joel/9a9fe2850c846e4184b491613f1d961e/external/+_repo_rules+xla 
    bazel build --run_under="cd /private/var/tmp/_bazel_joel/9a9fe2850c846e4184b491613f1d961e/external/+_repo_rules+xla" --enable_workspace --repo_env=HERMETIC_PYTHON_VERSION=3.11 //xla/extension:tarball
)"""

genrule(
    name = "build_ext",
    srcs = [],
    outs = ["xla_extension.tar.gz"],
    # cmd = "bazelisk --run_under='cd $(location @xla)' build @xla//xla/extension:tarball",
    cmd = cmd,
)
