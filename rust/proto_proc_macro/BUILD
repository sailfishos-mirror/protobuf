# An alternate implementation of the proto! macro as a proc macro instead of macro_rules.

load("@rules_rust//rust:defs.bzl", "rust_library", "rust_proc_macro")

rust_proc_macro(
    name = "proto_proc_macro_impl",
    srcs = ["proto_proc_macro_impl.rs"],
    deps = [
        "@crate_index//:proc_macro2",
        "@crate_index//:quote",
        "@crate_index//:syn",
    ],
)

rust_library(
    name = "proto_proc_macro",
    srcs = ["proto_proc_macro.rs"],
    proc_macro_deps = [":proto_proc_macro_impl"],
    visibility = ["//rust/test:__subpackages__"],
)
