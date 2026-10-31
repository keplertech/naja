# SPDX-FileCopyrightText: 2023 The Naja authors <https://github.com/najaeda/naja/blob/main/AUTHORS>
#
# SPDX-License-Identifier: Apache-2.0

"""Producer identity for root source builds and registered dependencies."""

def naja_git_hash():
    """Returns the commit a registry version of naja was built from.

    Registry entries for unreleased commits use BCR's
    `<release>-<YYYYMMDD>-<commit>` version scheme, so the hash is the
    last component. Anything else (a release, or naja as the root
    module) reports "unknown". Root builds use workspace status instead.
    Must be called from a BUILD file.
    """
    parts = native.module_version().split("-")
    if len(parts) == 3:
        return parts[2]
    return "unknown"


def _version_header_impl(ctx):
    inputs = [ctx.file.template]
    args = ctx.actions.args()
    args.add(ctx.file.template)
    args.add(ctx.outputs.out)
    args.add(ctx.attr.module_revision)
    if ctx.attr.workspace_identity:
        # Only Naja as the root may use this workspace's VCS identity. A
        # downstream consumer's status file describes the consumer, not Naja.
        inputs.append(ctx.info_file)
        args.add("--workspace-status", ctx.info_file)
    ctx.actions.run(
        executable = ctx.attr._generator[DefaultInfo].files_to_run,
        arguments = [args],
        inputs = inputs,
        outputs = [ctx.outputs.out],
        mnemonic = "NajaVersionHeader",
    )
    return [DefaultInfo(files = depset([ctx.outputs.out]))]

_version_header = rule(
    implementation = _version_header_impl,
    attrs = {
        "template": attr.label(allow_single_file = True, mandatory = True),
        "out": attr.output(mandatory = True),
        "module_revision": attr.string(mandatory = True),
        "workspace_identity": attr.bool(),
        "_generator": attr.label(
            default = Label(":generate_version_header"),
            executable = True,
            cfg = "exec",
        ),
    },
)


def naja_version_header(name):
    """Configure the header without running Git or host tools in build actions."""
    _version_header(
        name = name,
        template = "NajaVersion.h.in",
        out = "NajaVersion.h",
        module_revision = naja_git_hash(),
        workspace_identity = Label("//:MODULE.bazel").workspace_name == "",
    )
