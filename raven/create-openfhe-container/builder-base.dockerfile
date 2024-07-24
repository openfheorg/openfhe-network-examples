# gcc:9.3 is 400MB, jammy base is 30MB
#FROM ubuntu:jammy
#ENV HOME /
FROM gcc:9.4

# gcc is 180 MB, build-essential is 380 MB
RUN apt update && apt install -y \
        git \
	autoconf \
        cmake \
        build-essential \
        libtool \
        pkg-config

RUN git clone https://github.com/openfheorg/openfhe-development.git \
&& cd openfhe-development \
&& git checkout v1.2.0
WORKDIR /openfhe-development
RUN mkdir build
WORKDIR build
RUN cmake -DCMAKE_INSTALL_PREFIX=/opt/ ..
RUN make -j install

#install grpc
ENV MY_INSTALL_DIR=/opt/
WORKDIR /
ENV PATH="/opt/bin:$PATH"
RUN git clone --recurse-submodules -b v1.38.1 https://github.com/grpc/grpc
WORKDIR /grpc
RUN mkdir -p cmake/build
WORKDIR cmake/build
RUN echo $PATH
RUN echo $MY_INSTALL_DIR
RUN cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR ../..
RUN make -j
RUN make install
WORKDIR /grpc
RUN mkdir -p third_party/abseil-cpp/cmake/build
WORKDIR third_party/abseil-cpp/cmake/build
RUN cmake -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR -DCMAKE_POSITION_INDEPENDENT_CODE=TRUE ../..
RUN make -j
RUN make install
WORKDIR /openfhe-development/build

