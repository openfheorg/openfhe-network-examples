cc_binary(
    name = "pre_client",
    srcs = ["pre_client.cpp"],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//:grpc++",
        "//pre_net/protos:pre_net_cpp_grpc",
    ],
)

cc_binary(
    name = "pre_server",
    srcs = ["pre_server.cpp"],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//:grpc++",
        "//pre_net/protos:pre_net_cpp_grpc",
    ],
)
