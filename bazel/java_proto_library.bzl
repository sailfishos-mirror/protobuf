"""java_proto_library rule."""

load("//bazel/private:java_proto_library.bzl", _java_proto_library = "java_proto_library")

def java_proto_library(**kwattrs):
    # Only use Starlark rules when they are removed from Bazel
    if not hasattr(native, "java_proto_library"):
        _java_proto_library(**kwattrs)
    else:
        native.java_proto_library(**kwattrs)  # buildifier: disable=native-java-proto
