#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

#include "json_loader.h"
#include "logger.h"
#include "logging_request_handler.h"
#include "map_extra_data.h"
#include "player.h"
#include "records.h"
#include "request_handler.h"
#include "ticker.h"

namespace net = boost::asio;
namespace po = boost::program_options;
namespace fs = std::filesystem;

struct Args {
    std::optional<int> tick_period;
    std::filesystem::path config_file;
    std::filesystem::path www_root;
    bool randomize_spawn_points = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    Args args;

    po::options_description desc{"Allowed options"};

    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t",
            po::value<int>()->value_name("milliseconds"),
            "set tick period")
        ("config-file,c",
            po::value<std::string>()->value_name("file"),
            "set config file path")
        ("www-root,w",
            po::value<std::string>()->value_name("dir"),
            "set static files root")
        ("randomize-spawn-points",
            "spawn dogs at random positions");

    po::variables_map vm;

    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help")) {
        std::cout << desc << std::endl;
        return std::nullopt;
    }

    if (!vm.contains("config-file")) {
        throw std::runtime_error("Config file path is not specified");
    }

    if (!vm.contains("www-root")) {
        throw std::runtime_error("Static files root is not specified");
    }

    args.config_file = vm["config-file"].as<std::string>();
    args.www_root = vm["www-root"].as<std::string>();

    if (vm.contains("tick-period")) {
        const int period = vm["tick-period"].as<int>();

        if (period <= 0) {
            throw std::runtime_error("Tick period must be positive");
        }

        args.tick_period = period;
    }

    args.randomize_spawn_points = vm.contains("randomize-spawn-points");

    return args;
}

int main(int argc, const char* argv[]) {
    try {
        InitLogging();

        auto args_opt = ParseCommandLine(argc, argv);
        if (!args_opt) {
            return 0;
        }

        const Args& args = *args_opt;

        extra_data::Storage extra_storage;

        model::Game game = json_loader::LoadGame(
            args.config_file.string(),
            extra_storage
        );

        model::PlayerManager players;

        records::Repository records_repo{
            records::GetDbUrlFromEnv()
        };

        net::io_context ioc{1};

        http_handler::RequestHandler handler{
            game,
            players,
            args.www_root,
            extra_storage,
            records_repo,
            args.randomize_spawn_points,
            args.tick_period
        };

        LoggingRequestHandler logging_handler{handler};

        std::shared_ptr<Ticker> ticker;

        if (args.tick_period.has_value()) {
            auto api_strand = net::make_strand(ioc);

            ticker = std::make_shared<Ticker>(
                api_strand,
                std::chrono::milliseconds{*args.tick_period},
                [&handler](std::chrono::milliseconds delta) {
                    handler.Tick(delta);
                }
            );

            ticker->Start();
        }

        const net::ip::address address = net::ip::make_address("0.0.0.0");
        constexpr unsigned short port = 8080;

        BOOST_LOG_TRIVIAL(info)
            << boost::log::add_value(
                   additional_data,
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
                    std::forward<decltype(send)>(send)
                );
            }
        );

        ioc.run();
    } catch (const std::exception& ex) {
        BOOST_LOG_TRIVIAL(error)
            << boost::log::add_value(
                   additional_data,
                   boost::json::object{
                       {"exception", ex.what()}
                   })
            << "server exited with error";

        return 1;
    }
}
