#include "get_arg.hpp"

po::variables_map get_args(int argc, char* argv[]) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Show help message")
        ("config", po::value<string>()->default_value("config.toml"), "Configuration file")
    ;
    
    po::variables_map vm;

    try {
        po::store(
            po::command_line_parser(argc, argv)
                .options(desc)
                .extra_parser([](const string& s) -> pair<string, string> {
                    if (s == "-cfg") {
                        return make_pair(string("config"), string());
                    }
                    if (s.find("-cfg=") == 0) {
                        return make_pair(string("config"), s.substr(5));
                    }
                    return make_pair(string(), string());
                })
                .run(), 
            vm);
        po::notify(vm);
    } catch (const po::error& e) {
        cerr << "Error parsing command line: " << e.what() << endl;
        cerr << desc << endl;
        exit(1);
    }

    // Обработка --help
    if (vm.count("help")) {
        cout << desc << endl;
        exit(0);
    }

    return vm;
}