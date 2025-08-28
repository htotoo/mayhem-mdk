# Requirements
Like the mayhem-firmware a version of gcc-arm-none-eabi is required. See [Compile Firmware](https://github.com/portapack-mayhem/mayhem-firmware/wiki/Compile-firmware) on how to get started.

# Build
```
cmake -DCMAKE_TOOLCHAIN_FILE=config/arm-none-eabi-toolchain.cmake -B build -S.
make -C build uart_app
```


# Build with docker
```
docker build -t arm-docker-build config

docker run --rm -v .:/src -w /src arm-docker-build bash -c "cmake -DCMAKE_TOOLCHAIN_FILE=config/arm-none-eabi-toolchain.cmake -B build -S."

docker run --rm -v .:/src -w /src arm-docker-build bash -c "make -C build -j3 uart_app"
```
