docker build -t arm-docker-build config
docker run --rm -v .:/src -w /src arm-docker-build bash -c "cmake -DCMAKE_TOOLCHAIN_FILE=config/arm-none-eabi-toolchain.cmake -B build -S."
