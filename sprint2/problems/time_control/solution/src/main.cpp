#include "sdk.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <iostream>
#include <thread>

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "logger.h"

namespace net = boost::asio;

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json>\n";
        return 1;
    }

    try {
        InitLogging();

        model::Game game = json_loader::LoadGame(argv[1]);

        net::io_context ioc(1);

        http_handler::RequestHandler handler{game, argv[2]};

        LoggingRequestHandler logging_handler{handler};

        net::ip::address address = net::ip::make_address("0.0.0.0");
        const unsigned short port = 8080;

        BOOST_LOG_TRIVIAL(info)
            << boost::log::add_value(additional_data,
                boost::json::object{
                    {"port", port},
                    {"address", address.to_string()}
                })
            << "server started";

        http_server::ServeHttp(
            ioc,
            {address, port},
            [&logging_handler](auto&& req, auto&& send) {
                logging_handler(
                    std::forward<decltype(req)>(req),
                    std::forward<decltype(send)>(send));
            });

        ioc.run();

        BOOST_LOG_TRIVIAL(info)
            << boost::log::add_value(additional_data,
                boost::json::object{{"code", 0}})
            << "server exited";

    } catch (const std::exception& e) {

        BOOST_LOG_TRIVIAL(error)
            << boost::log::add_value(additional_data,
                boost::json::object{
                    {"code", 1},
                    {"exception", e.what()}
                })
            << "server exited";

        return 1;
    }
}
