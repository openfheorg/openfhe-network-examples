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

RUN git clone https://<username:accesstoken>@gitlab.com/duality-labs/palisade-serial-examples.git \
&& cd palisade-serial-examples
WORKDIR palisade-serial-examples
RUN mkdir build
WORKDIR build
RUN cmake -DWITH_OPENFHE=ON -DCMAKE_INSTALL_PREFIX=/opt/ ..
RUN make -j 32

RUN cp -r /palisade-serial-examples/demoData /opt/demoData/
RUN cp -r /palisade-serial-examples/NetworkMaps /opt/NetworkMaps/
RUN cp -r /palisade-serial-examples/scripts /opt/scripts
