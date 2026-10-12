# WIFI Evil Portal (standalone app for ESP32PP)

PortaPack standalone app (Utilities menu) that controls the ESP32PP
on-system Evil Portal / captive portal over I2C. Start/stop the portal,
pick a login template (Free WiFi / Google / Microsoft) and watch the live
captured-credential count. For authorized security-awareness / pentest use.

# Build

## with docker
docker build -t arm-docker-build ../common/config
docker run --rm -v $PWD:/src -v $PWD/../common/:/src/common/ -w /src arm-docker-build bash -c "cmake -DCMAKE_TOOLCHAIN_FILE=common/config/arm-none-eabi-toolchain.cmake -B build -S."
docker run --rm -v $PWD:/src -v $PWD/../common/:/src/common/ -w /src arm-docker-build bash -c "make -C build -j3"

Output: build/evilportal.ppmp and build/evilportal.h
Copy build/evilportal.h into ESP32-Portapack/Source/extapps/ and register
it with PPHandler::add_app to include it as an ext-app.
