#ifndef GET_CONFIG_HPP
#define GET_CONFIG_HPP

#include <iostream>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <toml++/toml.hpp>
#include <boost/regex.hpp>

using namespace std;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

struct Config {
    string input_dir;
    string output_dir;
    string config_file;
    vector<string> csv_filename_mask;
    vector<string> csv_files;
};

Config parsing(po::variables_map vm);

#endif