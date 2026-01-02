load("@bazel_skylib//rules:common_settings.bzl", "label_flag")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//visibility:public"],
)

label_flag(
    name = "proto_toolchain_for_java",
    build_setting_default = "@bazel_tools//tools/proto:java_toolchain",
)
