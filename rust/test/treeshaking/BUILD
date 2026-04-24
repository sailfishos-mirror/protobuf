load("@rules_rust//rust:defs.bzl", "rust_binary")
load("@rules_shell//shell:sh_test.bzl", "sh_test")
load("//bazel:proto_library.bzl", "proto_library")
load("//rust:defs.bzl", "rust_proto_library")

package(
    default_applicable_licenses = ["//:license"],
    default_visibility = ["//rust:__subpackages__"],
)

proto_library(
    name = "treeshaking_proto",
    testonly = True,
    srcs = ["treeshaking.proto"],
)

rust_proto_library(
    name = "treeshaking_rust_proto",
    testonly = True,
    deps = [":treeshaking_proto"],
)

rust_binary(
    name = "treeshaking_app",
    testonly = True,
    srcs = ["treeshaking_app.rs"],
    deps = [":treeshaking_rust_proto"],
)

sh_test(
    name = "rust_treeshaking_android_test",
    srcs = ["verify_treeshaking.sh"],
    args = [
        "$(rootpath :treeshaking_app)",
        "android",
        "$(rootpath //third_party/crosstool/google3_users:stable_nm)",
    ],
    data = [
        ":treeshaking_app",
        "//third_party/crosstool/google3_users:stable_nm",
    ],
    target_compatible_with = ["@platforms//os:android"],
)

sh_test(
    name = "rust_treeshaking_wasm_test",
    srcs = ["verify_treeshaking.sh"],
    args = [
        "$(rootpath :treeshaking_app)",
        "wasm",
        "$(rootpath //third_party/crosstool/google3_users:stable_nm)",
    ],
    data = [
        ":treeshaking_app",
        "//third_party/crosstool/google3_users:stable_nm",
    ],
    target_compatible_with = ["@platforms//os:emscripten"],
)
