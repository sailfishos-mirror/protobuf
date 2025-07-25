# Copyright (c) 2024, Google LLC
# All rights reserved.
#
# Use of this source code is governed by a BSD-style
# license that can be found in the LICENSE file or at
# https://developers.google.com/open-source/licenses/bsd

load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")
load(
    "//upb/bazel:build_defs.bzl",
    "UPB_DEFAULT_CPPOPTS",
)

package(default_applicable_licenses = ["//:license"])

cc_library(
    name = "template_help",
    hdrs = ["template_help.h"],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = ["//hpb:__subpackages__"],
    deps = [
        "//hpb:ptr",
    ],
)

cc_test(
    name = "template_help_test",
    srcs = ["template_help_test.cc"],
    deps = [
        ":template_help",
        "//hpb_generator/tests:test_model_hpb_proto",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)

cc_library(
    name = "internal",
    hdrs = ["internal.h"],
    visibility = [
        "//hpb:__subpackages__",
    ],
    deps = [
        "//upb/mem",
        "//upb/message",
    ],
)

cc_library(
    name = "message_lock",
    srcs = ["message_lock.cc"],
    hdrs = [
        "message_lock.h",
    ],
    copts = UPB_DEFAULT_CPPOPTS,
    visibility = [
        "//hpb:__subpackages__",
    ],
    deps = [
        "//hpb:status",
        "//upb/mem",
        "//upb/message",
        "//upb/message:copy",
        "//upb/message:promote",
        "//upb/mini_table",
        "//upb/wire",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings:string_view",
    ],
)

cc_test(
    name = "message_lock_test",
    srcs = ["message_lock_test.cc"],
    deps = [
        ":message_lock",
        "//hpb",
        "//hpb:arena",
        "//hpb:extension",
        "//hpb_generator/tests:test_model_hpb_proto",
        "//upb/mem",
        "//upb/mini_table",
        "@abseil-cpp//absl/hash",
        "@abseil-cpp//absl/log:absl_check",
        "@googletest//:gtest",
        "@googletest//:gtest_main",
    ],
)
