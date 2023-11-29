FROM palisades-library:jammy

RUN apt-get update && apt-get install -y \
	build-essential \
	git \
	make \
	cmake \
	autoconf \
        vim \
        openssl \
        ca-certificates \
	libboost-all-dev \
        gcc-9 \
        g++-9 \
        && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 100 \
        && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 100

RUN apt-get update && apt-get install -y libgomp1

ENV PATH="/opt/bin:$PATH"

RUN git clone https://github.com/openfheorg/openfhe-network-examples.git \
&& cd openfhe-network-examples
WORKDIR openfhe-network-examples
RUN mkdir build
WORKDIR build
RUN cmake -DWITH_OPENFHE=ON -DCMAKE_INSTALL_PREFIX=/opt/ ..
RUN make -j 32

RUN cp -r /openfhe-network-examples/demoData /opt/demoData/
RUN cp -r /openfhe-network-examples/NetworkMaps /opt/NetworkMaps/
RUN cp -r /openfhe-network-examples/scripts /opt/scripts
