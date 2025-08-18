load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def _xla_download_impl(ctx):
    url = "https://github.com/openxla/xla/archive/{}.tar.gz".format(ctx.attr.commit)
    ctx.download_and_extract(
        url,
        sha256 = ctx.attr.sha256,
        stripPrefix = "xla-{}".format(ctx.attr.commit),
    )

    target = ctx.workspace_root.get_child("extension")
    source = ctx.path("xla/extension")
    ctx.symlink(target, source)

xla_download = repository_rule(
    implementation = _xla_download_impl,
    attrs = {
        "commit": attr.string(
            mandatory = True,
            doc = "XLA commit to be downloaded",
        ),
        "sha256": attr.string(
            mandatory = True,
            doc = "SHA256 hash of the XLA tarball",
        ),
    },
    doc = "Download XLA distribution",
)

def xla_repo():
    xla_download(
        name = "xla",
        commit = "2a6015f068e4285a69ca9a535af63173ba92995b",
        sha256 = "dd23128eb781e83b348612e5d944cab730efe7c95497c51d28dcaedd63bef679",
    )

def _xla_extension_impl(_ctx):
    xla_repo()

xla_ext = module_extension(
    implementation = _xla_extension_impl,
)
