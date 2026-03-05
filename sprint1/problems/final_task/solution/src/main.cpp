#include "http_server.h"
#include "request_handler.h"
#include "json_loader.h"

#include <boost/asio.hpp>
#include <iostream>

namespace net = boost::asio;
using tcp = net::ip::tcp;

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: game_server <config>" << std::endl;
        return 1;
    }

    try {
        net::io_context ioc{1};

        auto game = json_loader::LoadGame(argv[1]);

        http_handler::RequestHandler handler{game};

        tcp::endpoint endpoint{tcp::v4(), 8080};

        std::cout << "Server started" << std::endl;

        http_server::ServeHttp(
            ioc,
            endpoint,
            [&handler](auto&& req, auto&& send) {
                handler(std::forward<decltype(req)>(req),
                        std::forward<decltype(send)>(send));
            });

        ioc.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }
}
