# ======== Build stage ========
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y python3-pip cmake && \
    pip3 install conan==2.*

RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.4/cmake-3.27.4-linux-x86_64.sh && \
    chmod +x cmake-3.27.4-linux-x86_64.sh && \
    ./cmake-3.27.4-linux-x86_64.sh --skip-license --prefix=/usr/local && \
    rm cmake-3.27.4-linux-x86_64.sh

WORKDIR /app

COPY conanfile.txt /app/

RUN conan profile detect

RUN mkdir build && cd build && \
    conan install .. --build=missing

COPY ./src /app/src
COPY CMakeLists.txt /app/

RUN cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake .. && \
    cmake --build .

FROM ubuntu:22.04

RUN groupadd -r www && useradd -r -g www www
USER www

WORKDIR /app

COPY --from=build /app/build/game_server /app/game_server
COPY ./data /app/data

ENTRYPOINT ["/app/game_server", "/app/data/config.json"]