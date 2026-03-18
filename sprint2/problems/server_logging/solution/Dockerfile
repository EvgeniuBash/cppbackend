# ==========================
# Stage 1: Build
# ==========================
FROM gcc:11.3 AS build

# Устанавливаем зависимости
RUN apt update && \
    apt install -y python3-pip cmake && \
    pip3 install conan==1.*

# Создаём рабочую директорию
WORKDIR /app

# Копируем Conan файл и устанавливаем зависимости
COPY conanfile.txt .
RUN mkdir build && cd build && \
    conan install .. --build=missing -g cmake

# Копируем исходники и CMakeLists
COPY ./src ./src
COPY CMakeLists.txt .

# Сборка
RUN cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release && \
    cmake --build .

# ==========================
# Stage 2: Runtime
# ==========================
FROM ubuntu:22.04 AS run

# Создаём пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Копируем бинарник и данные
COPY --from=build /app/build/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

# Запуск приложения
ENTRYPOINT ["/app/game_server", "/app/data/config.json", "/app/static/"]