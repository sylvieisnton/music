#pragma once

#include <iostream>
#include <stdexcept>
#include <string>

struct Options {
    std::string input;
    std::string output;
    bool help = false;
};

inline void help(const char* program) {
    std::cout
    << "Usage: " << program << " -i <input> -o <output>\n"
    << "  -i, --input     input file\n"
    << "  -o, --output    output file\n"
    << "  -h, --help      show this menu\n";
}

inline Options parse(int argc, char* argv[]) {
    Options options;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            options.help = true;
        }
        if (arg == "-i" || arg == "--input") {
            if (i + 1 < argc) {
                options.input = argv[++i];
            } else {
                throw std::invalid_argument("Missing input file");
            }
        }
        if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                options.output = argv[++i];
            } else {
                throw std::invalid_argument("Missing output file");
            }
        }

        std::string value = argv[i+1];

        if (value.empty() || value[0] == '-') {
            throw std::invalid_argument("Missing value for " + arg);
        }

        ++i;

        std::string& destination = (arg == "-i" || arg == "--input") ? options.input : options.output;

        if (!destination.empty()) {
            throw std::invalid_argument("Duplicate argument: " + arg);
        }

        destination = value;
    }

    if (options.input.empty() || options.output.empty()) {
        options.help = true;
    }

    return options;
}