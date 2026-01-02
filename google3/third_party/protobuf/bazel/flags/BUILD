load("@bazel_skylib//rules:common_settings.bzl", "bool_flag", "label_flag", "string_flag", "string_list_flag")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

string_list_flag(
    name = "use_starlark_flags",
    build_setting_default = [],
)

bool_flag(
    name = "experimental_proto_descriptor_sets_include_source_info",
    build_setting_default = False,
)

label_flag(
    name = "proto_compiler",
    build_setting_default = "@bazel_tools//tools/proto:protoc",
)

string_flag(
    name = "strict_proto_deps",
    build_setting_default = "error",
)

string_flag(
    name = "strict_public_imports",
    build_setting_default = "off",
)
