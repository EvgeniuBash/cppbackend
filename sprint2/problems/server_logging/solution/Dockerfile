FROM gcc:11.3 AS build

RUN apt update && \
    apt install -y python3-pip cmake libicu-dev && \
    pip3 install conan==1.*

WORKDIR /app

COPY conanfile.txt .
RUN mkdir build && cd build && \
    conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11

COPY ./src ./src
COPY CMakeLists.txt .

RUN cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && cmake --build .

FROM ubuntu:22.04 AS run

RUN groupadd -r www && useradd -r -g www www
USER www

WORKDIR /app

COPY --from=build /app/build/bin/game_server ./game_server
COPY ./data ./data
COPY ./static ./static

ENTRYPOINT ["./game_server", "./data/config.json", "./static/"]