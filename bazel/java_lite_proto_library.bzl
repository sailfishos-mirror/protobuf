"""java_lite_proto_library rule"""

load("//bazel/private:java_lite_proto_library.bzl", _java_lite_proto_library = "java_lite_proto_library")

def java_lite_proto_library(**kwattrs):
    # Only use Starlark rules when they are removed from Bazel
    if not hasattr(native, "java_lite_proto_library"):
        _java_lite_proto_library(**kwattrs)
    else:
        native.java_lite_proto_library(**kwattrs)  # buildifier: disable=native-java-lite-proto
