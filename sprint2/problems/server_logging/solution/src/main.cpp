#include "logger.h"
#include "request_handler.h"
#include "json_loader.h"
#include "http_server.h"
#include <boost/asio.hpp>

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-root>\n";
        return 1;
    }

    try {
        auto game = json_loader::LoadGame(argv[1]);

        boost::asio::io_context ioc(1);

        init_logging();
        log_server_started(8080, "0.0.0.0");

        http_handler::RequestHandler handler{game, argv[2]};
        LoggingRequestHandler<http_handler::RequestHandler> logging_handler{handler};

        boost::asio::ip::address address = boost::asio::ip::make_address("0.0.0.0");
        unsigned short port = 8080;

        http_server::ServeHttp(
            ioc,
            {address, port},
            [&logging_handler](auto&& req, auto&& send) {
                logging_handler(std::forward<decltype(req)>(req),
                                std::forward<decltype(send)>(send));
            });

        ioc.run();
        log_server_exited(0);

    } catch (const std::exception& e) {
        log_server_exited(EXIT_FAILURE, e.what());
        return 1;
    }
}
