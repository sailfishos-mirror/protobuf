"""Functionality to turn on and off Starlark implementations of proto flags."""

load("@bazel_skylib//rules:common_settings.bzl", "BuildSettingInfo")

def _command_line_setting(use_starlark_flags, flag_name):
    """Returns if --flag_name should use the Starlark or native definition.

    Rule logic can set its own defaults, for when this flag isn't set.

    Args:
      use_starlark_flags: Value of --//mylang:flags:use_starlark_flags
      flag_name: name of the flag to check, minus "--". Example: "javacopt".

    Returns:
      "starlark": use the Starlark definition
      "native": use the native definition
      "language default": use the rule set's default choice
    """
    if not use_starlark_flags:
        return "language default"  # --use_starlark_flags isn't set.
    elif len(use_starlark_flags) == 1 and use_starlark_flags[0] == "*":
        return "starlark"  # --use_starlark_flags=* means "enable all flags".
    elif len(use_starlark_flags) == 1 and use_starlark_flags[0] == "-":
        return "native"  # --use_starlark_flags=- means "disable all flags".
    elif flag_name in use_starlark_flags:
        return "starlark"  # --use_starlark_flags=foo means "enable --foo".
    elif "-" + flag_name in use_starlark_flags:
        return "native"  # --use_starlark_flags=-foo means "disable --foo".
    else:
        return "language default"  # --use_starlark_flags=otherflag1,otherflag2

# Maps flag names to (starlark|native, native reference)
#
# starlark|native: language policy on using Starlark vs. native definition
#   if user doesn't override with --//mylang:flags:use_starlark_flags.
# native reference: Starlark API reference to native flag
_FLAG_DEFINITIONS = {
    "protocopt": (
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "experimental_protoc_opts"),
    ),
    "experimental_proto_descriptor_sets_include_source_info": (
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "experimental_proto_descriptor_sets_include_source_info"),
    ),
    "strict_proto_deps": (
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "strict_proto_deps"),
    ),
    "strict_public_imports": (
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "strict_public_imports"),
    ),
    "cc_proto_library_header_suffixes": (
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "cc_proto_library_header_suffixes"),
    ),
    "cc_proto_library_source_suffixes": (
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "cc_proto_library_source_suffixes"),
    ),
    "proto_toolchain_for_cc": (
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "proto_toolchain_for_cc"),
    ),
    "proto_compiler": (
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "proto_compiler"),
    ),
    "proto_toolchain_for_java": (
        # this is _flag in the code. figure thisout
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "proto_toolchain_for_java"),
    ),
    "proto_toolchain_for_javalite": (
        # this is _flag in the code. figure thisout
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "proto_toolchain_for_javalite"),
    ),
    "proto_toolchain_for_j2objc": (
        # this is _flag in the code. figure thisout
        "native",
        lambda ctx: getattr(ctx.fragments.proto, "proto_toolchain_for_j2objc"),
    ),
}

def get_flag_value(ctx, flag_name):
    flag_source_of_truth = _command_line_setting(
        # See previous code block.
        getattr(ctx.attr, "_use_starlark_flags")[BuildSettingInfo].value,
        flag_name,
    )
    if flag_source_of_truth == "language default":
        if flag_name in _FLAG_DEFINITIONS:
            flag_source_of_truth = _FLAG_DEFINITIONS[flag_name][0]
        else:
            flag_source_of_truth = "native"

        return _FLAG_DEFINITIONS[flag_name][1](ctx)
