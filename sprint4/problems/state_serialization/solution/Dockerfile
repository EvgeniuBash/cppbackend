FROM ubuntu:22.04

WORKDIR /app

COPY build/game_server /app/game_server
COPY data /app/data
COPY static /app/static

EXPOSE 8080

ENTRYPOINT ["/app/game_server"]