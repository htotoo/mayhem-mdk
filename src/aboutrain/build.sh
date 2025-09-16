
docker run --rm -v $PWD:/src -v $PWD/../common/:/src/common/ -w /src arm-docker-build bash -c "cmake -DCMAKE_TOOLCHAIN_FILE=common/config/arm-none-eabi-toolchain.cmake -B build -S."
docker run --rm -v $PWD:/src -v $PWD/../common/:/src/common/ -w /src arm-docker-build bash -c "make -C build -j3 aboutrain"