def _collect_static_impl(ctx):
    libs = []
    for d in ctx.attr.deps:
        cc = d[CcInfo]
        for li in cc.linking_context.linker_inputs.to_list():
            for l in li.libraries:
                if l.static_library:
                    libs.append(l.static_library)
                if l.pic_static_library:
                    libs.append(l.pic_static_library)
    return [DefaultInfo(files = depset(libs))]

collect_static = rule(
    implementation = _collect_static_impl,
    attrs = {
        "deps": attr.label_list(providers = [CcInfo]),
    },
)
