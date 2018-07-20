
OPTIONS=--copt=-mavx --copt=-mavx2 --copt=-mfma --copt=-msse4.1 --copt=-msse4.2

all:
	cd ../../../ && bazel build --verbose_failures $(OPTIONS) tensorflow/examples/ch-storage-server/... && cd -

