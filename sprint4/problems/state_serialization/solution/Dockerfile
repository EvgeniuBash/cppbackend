FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    libboost-all-dev

WORKDIR /app

COPY . .

RUN cmake -B build
RUN cmake --build build

CMD ["./build/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static/"]