#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <optional>
#include <vector>

using namespace std::literals;

struct Args {
    std::optional<int> tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn = false;
}; 

std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    Args args;

    po::options_description desc{"Allowed options"s};

    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t",
            po::value<int>()->value_name("milliseconds"),
            "set tick period")
        ("config-file,c",
            po::value<std::string>(&args.config_file)->value_name("file"),
            "set config file path")
        ("www-root,w",
            po::value<std::string>(&args.www_root)->value_name("dir"),
            "set static files root")
        ("randomize-spawn-points",
            po::bool_switch(&args.randomize_spawn),
            "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cout << desc << std::endl;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("config-file is required"s);
    }

    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("www-root is required"s);
    }

    // tick-period (опциональный)
    if (vm.contains("tick-period"s)) {
        args.tick_period = vm["tick-period"].as<int>();
    }

    return args;
}