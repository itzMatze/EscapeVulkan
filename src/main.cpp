#include <boost/program_options/options_description.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options.hpp>
#include <vector>

#include "vk/Timer.hpp"
#include "MainContext.hpp"

int parse_args(int argc, char** argv, boost::program_options::variables_map& vm)
{
    namespace bpo = boost::program_options;
    bpo::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Print this help message")
        ("nn_file", bpo::value<std::string>(), "Load neural network checkpoint from given file and initialize the agent with this network")
        ("train_mode,T", "Train agent")
        ("disable_rendering,R", "Do not render the game to enable quicker training iterations")
    ;
    bpo::store(bpo::parse_command_line(argc, argv, desc), vm);
    bpo::notify(vm);
    if (vm.count("help"))
    {
        std::cout << desc << std::endl;
        return 1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    boost::program_options::variables_map vm;
    int parse_status = parse_args(argc, argv, vm);
    if (parse_status != 0) return parse_status;
    std::string nn_file = "";
    if (vm.count("nn_file")) nn_file = vm["nn_file"].as<std::string>();
    bool train_mode = vm.count("train_mode");
    bool disable_rendering = vm.count("disable_rendering");

    std::vector<spdlog::sink_ptr> sinks;
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_sink_st>());
    sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_st>("ve.log", true));
    auto combined_logger = std::make_shared<spdlog::logger>("default_logger", sinks.begin(), sinks.end());
    spdlog::set_default_logger(combined_logger);
    spdlog::set_level(spdlog::level::debug);
    spdlog::set_pattern("[%Y-%m-%d %T.%e] [%L] %v");
    spdlog::info("Starting");
    ve::HostTimer t;
    MainContext mc(nn_file, train_mode, disable_rendering);
    spdlog::info("Setup took: {} ms", t.elapsed());
    mc.run();
    return 0;
}
