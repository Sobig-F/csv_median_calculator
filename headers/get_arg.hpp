#ifndef GET_ARG_HPP
#define GET_ARG_HPP

#include <iostream>
#include <boost/program_options.hpp>

using namespace std;

namespace po = boost::program_options;

po::variables_map get_args(int argc, char* argv[]);

#endif