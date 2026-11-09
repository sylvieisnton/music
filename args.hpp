#pragma once

#include <iostream>
#include <stdexcept>
#include <string>

struct Options
{
    std::string input;
    std::string output;
    std::string speed = "normal";
    std::string scale = "chromatic";
    bool help = false;
};

inline void help(const char *program)
{
    std::cout
        << "Usage: " << program << " -i <input> -o <output>\n"
        << "  -i, --input     input file\n"
        << "  -o, --output    output file\n"
        << "  -s, --speed     slow, normal, fast (default: normal)\n"
        << "  -sc, --scale    chromatic, pentatonic, minor\n"
        << "  -h, --help      show this menu\n";
}

inline Options parse(int argc, char *argv[])
{
    Options options;

    bool speed = false;
    bool scale = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            options.help = true;
            continue;
        }

        if (arg == "-s" || arg == "--speed")
        {
            if (speed)
            {
                throw std::invalid_argument(
                    "Duplicate argument: " + arg);
            }

            if (i + 1 >= argc)
            {
                throw std::invalid_argument(
                    "Missing value for " + arg);
            }

            options.speed = argv[++i];

            if (options.speed != "slow" &&
                options.speed != "normal" &&
                options.speed != "fast")
            {
                throw std::invalid_argument(
                    "Speed must be slow, normal, or fast");
            }

            speed = true;
            continue;
        }

        if (arg == "-sc" || arg == "--scale")
        {
            if (scale)
            {
                throw std::invalid_argument(
                    "Duplicate argument: " + arg);
            }

            if (i + 1 >= argc)
            {
                throw std::invalid_argument(
                    "Missing value for " + arg);
            }

            options.scale = argv[++i];

            if (options.scale != "chromatic" &&
                options.scale != "pentatonic" &&
                options.scale != "minor")
            {
                throw std::invalid_argument(
                    "Scale must be chromatic, pentatonic, or minor");
            }

            scale = true;
            continue;
        }

        if (arg != "-i" && arg != "--input" && arg != "-o" && arg != "--output")
        {
            throw std::invalid_argument("Unknown argument: " + arg);
        }

        if (i + 1 >= argc)
        {
            throw std::invalid_argument("Missing value for " + arg);
        }

        std::string value = argv[i + 1];

        if (value.empty() || value[0] == '-')
        {
            throw std::invalid_argument("Missing value for " + arg);
        }

        std::string &destination = (arg == "-i" || arg == "--input") ? options.input : options.output;

        if (!destination.empty())
        {
            throw std::invalid_argument("Duplicate argument: " + arg);
        }

        destination = value;
        ++i;
    }

    if (options.input.empty() || options.output.empty())
    {
        options.help = true;
    }

    return options;
}