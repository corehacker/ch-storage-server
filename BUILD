# Description:
#   TensorFlow C++ inference example for labeling images.

package(
    default_visibility = ["//tensorflow:internal"],
)

licenses(["notice"])  # Apache 2.0

exports_files(["LICENSE"])

load("//tensorflow:tensorflow.bzl", "tf_cc_binary")

tf_cc_binary(
    name = "ch-storage-server",
    srcs = [
        "src/main.cpp", 
        "src/storage-server.cpp", 
        "src/config.cpp", 
        "src/motion-detector.cpp", 
        "src/mail-client.cpp", 
        "src/curl-smtp.cpp",
        "src/storage-server.hpp", 
        "src/config.hpp", 
        "src/motion-detector.hpp", 
        "src/mail-client.hpp", 
        "src/curl-smtp.hpp",
        "src/label-image.cpp",
        "src/label-image.hpp",
        "src/kafka-client.cpp",
        "src/kafka-client.hpp",
        "src/firebase-client.cpp",
        "src/firebase-client.hpp"
    ],
    copts = ["-fexceptions", "-DENABLE_TENSORFLOW"],
    linkopts = ["-lm", "-lcurl",
        "-lavformat -lavcodec -lavutil -lswscale -lswresample",
        "-L/usr/local/lib",
        "-lglog", "-levent", 
        "-lopencv_core", "-lopencv_videoio", "-lopencv_ccalib", "-lopencv_highgui", 
        "-lopencv_imgproc", "-lopencv_video", "-lopencv_bgsegm", "-lopencv_imgcodecs",
        "-lrdkafka++", "-lch-cpp-utils"],
    deps = [
            "//tensorflow/cc:cc_ops",
            "//tensorflow/core:core_cpu",
            "//tensorflow/core:framework",
            "//tensorflow/core:framework_internal",
            "//tensorflow/core:lib",
            "//tensorflow/core:protos_all_cc",
            "//tensorflow/core:tensorflow",
    ],
)
