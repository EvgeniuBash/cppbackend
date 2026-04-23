#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include <memory>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include "json_loader.h"
#include "request_handler.h"
#include "player.h"
#include "ticker.h"
#include "logger.h"
#include "logging_request_handler.h"
#include "map_extra_data.h"
#include "db.h"

namespace net = boost::asio;
namespace po = boost::program_options;
namespace fs = std::filesystem;

using namespace std::literals;

struct Args {
    std::optional<int> tick_period;
    std::filesystem::path config_file;
    std::filesystem::path www_root;
    bool randomize_spawn_points = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

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
        int period = vm["tick-period"].as<int>();
        if (period <= 0) {
            throw std::runtime_error("Tick period must be positive");
        }
        args.tick_period = period;
    }

    args.randomize_spawn_points = vm.contains("randomize-spawn-points");

    return args;
}

void TickPlayers(model::PlayerManager& players, std::chrono::milliseconds delta) {
    const double dt = delta.count() / 1000.0;

    for (auto* player : players.GetAllPlayers()) {
        auto pos = player->GetPosition();
        auto speed = player->GetSpeed();

        pos.x += speed.vx * dt;
        pos.y += speed.vy * dt;

        player->SetPosition(pos);
    }
}

int main(int argc, const char* argv[]) {
    try {
        InitLogging();

        const char* db_url = std::getenv("GAME_DB_URL");
        if (!db_url) {
            throw std::runtime_error("GAME_DB_URL not set");
        }

        Database db(db_url);
        db.Init();

        auto args_opt = ParseCommandLine(argc, argv);
        if (!args_opt) return 0; 
        const Args& args = *args_opt;
        extra_data::Storage extra_storage;
       
        model::Game game = json_loader::LoadGame(args.config_file.string(), extra_storage);
        model::PlayerManager players;

        net::io_context ioc(1);

        http_handler::RequestHandler handler{
            game,
            players,
            args.www_root,
            extra_storage,
            args.randomize_spawn_points,
            args.tick_period
        };
        LoggingRequestHandler logging_handler{handler};

        using namespace std::chrono;
        std::shared_ptr<Ticker> ticker;

        if (args.tick_period) {
            auto api_strand = net::make_strand(ioc);
            const int retirement_time = 60;
            ticker = std::make_shared<Ticker>(
                api_strand,
                milliseconds(*args.tick_period),
                [&players, &db, retirement_time](milliseconds delta) {
                    TickPlayers(players, delta);
                    auto retired = players.RemoveInactive(std::chrono::seconds(retirement_time));

                    for (auto* p : retired) {
                        double play_time = std::chrono::duration<double>(
                            model::Player::Clock::now() - p->GetJoinTime()
                        ).count();

                        db.AddRecord(p->GetName(), p->GetScore(), play_time);
                    }
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
