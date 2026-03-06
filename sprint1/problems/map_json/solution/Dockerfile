FROM gcc:11.3 as build

RUN apt update && \
    apt install -y python3-pip cmake && \
    pip3 install conan==1.*

WORKDIR /app

COPY conanfile.txt .
RUN mkdir build && cd build && \
    conan install .. --build=missing

COPY src ./src
COPY CMakeLists.txt .
COPY data ./data

RUN cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build .

FROM ubuntu:22.04 as run

RUN groupadd -r www && useradd -r -g www www
WORKDIR /app

COPY --from=build /app/build/bin/game_server /app/game_server
COPY --from=build /app/data /app/data

USER www

EXPOSE 8080

ENTRYPOINT ["/app/game_server", "/app/data/config.json"]