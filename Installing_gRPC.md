Installing gRPC on your system
=====

Installing prerequisites (from https://grpc.io/docs/languages/cpp/quickstart/)
---

Choose a directory to hold locally installed packages using an
appropriately set `CMAKE_INSTALL_PREFIX`, because there is no easy way
to uninstall gRPC after you've installed it globally. For example
`~/.local/`

	export MY\_INSTALL\_DIR=$HOME/.local

Ensure that the directory exists	

	mkdir -p $MY\_INSTALL\_DIR

Check if `"$MY\_INSTALL\_DIR/bin"` is in your `$PATH`. If not then add
the local bin folder to your path variable

	export PATH="$MY\_INSTALL\_DIR/bin:$PATH"


Check cmake version (`cmake --version`) or install `cmake` and check its
version. It should be 3.13 or later
	
	sudo apt install -y cmake

If `cmake`'s version is still old, reinstall it	

	wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.19.6/cmake-3.19.6-Linux-x86_64.sh
	sh cmake-linux.sh -- --skip-license --prefix=$MY\_INSTALL\_DIR
	rm cmake-linux.sh

Install other tools	

	sudo apt install -y build-essential autoconf libtool pkg-config
		
Building and Installing gRPC, Protocol Buffers, and Abseil
---

Check for the current version of grpc and clone the repo including all necessary 3rd party libs:

`https://grpc.io/docs/languages/cpp/quickstart/#:~:text=git%20clone%20--recurse-submodules%20-b%20v1.38.0%20https%3A//github.com/grpc/grpc`

	git clone --recurse-submodules -b v1.38.1 https://github.com/grpc/grpc
	cd grpc
	mkdir -p cmake/build
	pushd cmake/build
	cmake -DgRPC\_INSTALL=ON -DgRPC\_BUILD\_TESTS=OFF -DCMAKE\_INSTALL\_PREFIX=$MY\_INSTALL\_DIR ../..
	make -j
	make install
	popd
	mkdir -p third\_party/abseil-cpp/cmake/build
	pushd third\_party/abseil-cpp/cmake/build
	cmake -DCMAKE\_INSTALL\_PREFIX=$MY\_INSTALL\_DIR -DCMAKE\_POSITION\_INDEPENDENT\_CODE=TRUE ../..
	make -j
	make install
	popd
