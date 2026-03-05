#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "http_server.h"

namespace net = boost::asio;

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: game_server <game-config-json>\n";
        return 1;
    }

    try {
        model::Game game = json_loader::LoadGame(argv[1]);

        net::io_context ioc(1);

        http_handler::RequestHandler handler{game};

        net::ip::address address = net::ip::make_address("0.0.0.0");
        const unsigned short port = 8080;

        http_server::ServeHttp(
            ioc,
            {address, port},
            [&handler](auto&& req, auto&& send) {
                handler(std::forward<decltype(req)>(req),
                        std::forward<decltype(send)>(send));
            });

        std::cout << "Server has started..." << std::endl;

        ioc.run();

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
