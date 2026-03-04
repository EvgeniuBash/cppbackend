#include "json_loader.h"
#include <iostream>

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: game_server <game-config-json>" << std::endl;
        return 1;
    }

    try {
        model::Game game = json_loader::LoadGame(argv[1]);

        // В map_json сервер не запускаем!
        // Просто сообщаем, что всё успешно

        std::cout << "Server has started..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
