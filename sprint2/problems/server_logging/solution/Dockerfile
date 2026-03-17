FROM gcc:11.3 as build

RUN apt update && \
    apt install -y python3-pip cmake && \
    pip3 install "conan>=2.0,<3.0"

WORKDIR /app

COPY conanfile.txt /app/

RUN conan profile detect

RUN mkdir build && cd build && \
    conan install .. --build=missing

COPY ./src /app/src
COPY CMakeLists.txt /app/

# Используем toolchain от Conan 2
RUN cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake .. && \
    cmake --build .

FROM ubuntu:22.04 as run

RUN groupadd -r www && useradd -r -g www www
USER www

WORKDIR /app
COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data

ENTRYPOINT ["/app/game_server", "/app/data/config.json"]