# to build

cmake -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi-toolchain.cmake -B build -S.
make -C build uart_app


# build with docker
docker build -t arm-docker-build config
docker run --rm -v .:/src -w /src arm-docker-build bash -c "cmake -DCMAKE_TOOLCHAIN_FILE=config/arm-none-eabi-toolchain.cmake -B build -S."
docker run --rm -v .:/src -w /src arm-docker-build bash -c "make -C build -j3 uart_app"
