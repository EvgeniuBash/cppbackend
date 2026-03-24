#include "sdk.h"
#include "merge.cpp" // здесь ParseCommandLine и Args
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <iostream>
#include <thread>
#include <memory>

#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "logger.h"

namespace net = boost::asio;

int main(int argc, const char* argv[]) {
    try {
        InitLogging();

        auto args = ParseCommandLine(argc, argv);
        if (!args) return 0; // help

        model::Game game = json_loader::LoadGame(args->config_file);

        net::io_context ioc(1);

        http_handler::RequestHandler handler{
            game,
            args->www_root,
            args->randomize_spawn,
            args->tick_period
        };
        LoggingRequestHandler logging_handler{handler};

        using namespace std::chrono;
        std::shared_ptr<Ticker> ticker;

        if (args->tick_period) {
            auto api_strand = net::make_strand(ioc);
            ticker = std::make_shared<Ticker>(
                api_strand,
                milliseconds(*args->tick_period),
                [&game](milliseconds delta) {
                    game.Tick(delta); // авто-тикинг игры
                }
            );
            ticker->Start();
        }

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
            }
        );

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
