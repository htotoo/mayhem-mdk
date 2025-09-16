# A test app to check if map ui is working in the standalone apps. 


# Requirements
Like the mayhem-firmware a version of gcc-arm-none-eabi is required. See [Compile Firmware](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Compile-firmware) on how to get started.

# to build

cmake -DCMAKE_TOOLCHAIN_FILE=../common/config/arm-none-eabi-toolchain.cmake -B build -S.
make -C build


# build with docker
docker build -t arm-docker-build ../common/config
docker run --rm -v .:/src -w /src arm-docker-build bash -c "cmake -DCMAKE_TOOLCHAIN_FILE=common/config/arm-none-eabi-toolchain.cmake -B build -S."
docker run --rm -v .:/src -w /src arm-docker-build bash -c "make -C build -j3"
