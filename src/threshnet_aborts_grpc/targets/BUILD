cc_binary(
    name = "thresh_client",
    srcs = ["thresh_client.cpp"],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//:grpc++",
        "//thresh_net_aborts_grpc/protos:thresh_net_cpp_grpc",
    ],
)

cc_binary(
    name = "thresh_server",
    srcs = ["thresh_server.cpp"],
    defines = ["BAZEL_BUILD"],
    deps = [
        "//:grpc++",
        "//thresh_net_aborts_grpc/protos:thresh_net_cpp_grpc",
    ],
)
