#include "config.hpp"

Config parsing(po::variables_map vm) {

    Config config;

    // Проверка существования конфигурационного файла
    string config_path = vm["config"].as<string>();
    if (!fs::exists(config_path)) {
        cerr << "Error: Configuration file '" << config_path << "' not found!" << endl;
        exit(1);
    }
    
    config.config_file = config_path;

    // Парсинг TOML файла
    try {
        auto toml_file = toml::parse_file(config.config_file);
        
        config.input_dir = string(toml_file["main"]["input"].value_or(""));
        config.output_dir = string(toml_file["main"]["output"].value_or(""));
        config.csv_filename_mask = [&]() -> vector<string> {
            vector<string> result;
            if (auto arr = toml_file["main"]["filename_mask"].as_array()) {
                for (auto& elem : *arr) {
                    if (auto str = elem.value<string>()) {
                        result.push_back(*str);
                    }
                }
            }
            return result;
        }();
        config.csv_files = [&]() -> vector<string> {
            vector<string> result;
            fs::path dir(config.input_dir);
            boost::regex mask;

            for (string conf_mask : config.csv_filename_mask) {
                mask = boost::regex(".*" + conf_mask + ".*\\.csv$", boost::regex::icase);
                for (const auto& entry : fs::directory_iterator(dir)) {
                    if (fs::is_regular_file(entry.path())) {
                        std::string filename = entry.path().filename().string();
                        if (boost::regex_match(filename, mask)) {
                            result.push_back(entry.path().string());
                        }
                    }
                }
            }
            sort(result.begin(), result.end());
            result.erase(unique(result.begin(), result.end()), result.end());

            return result;
        }();
    } catch (const toml::parse_error& err) {
        cerr << "Error parsing TOML file: " << err << endl;
        exit(1);
    }

    return config;
}