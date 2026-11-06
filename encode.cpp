#include "args.hpp"
#include "checksum.hpp"

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstdint>

double frequency(int note)
{
    return 440.0 * std::pow(2.0, (note) / 12.0);
}

constexpr int sampleRate = 44100;
constexpr double pi = 3.14159265358979323846;

std::vector<std::int16_t> generateTone(double frequency)
{
    constexpr int sampleCt = sampleRate / 10;
    std::vector<std::int16_t> samples(sampleCt);

    for (int i = 0; i < sampleCt; ++i)
    {
        double time = static_cast<double>(i) / sampleRate;
        double wave = std::sin(2.0 * pi * frequency * time);
        double envelope = std::sin(pi * i / (sampleCt - 1));
        envelope = envelope * envelope;

        samples[i] = static_cast<std::int16_t>(
            wave * envelope * 8000.0);
    }
    return samples;
}

void m16(std::ostream &out, std::uint16_t value)
{
    out.put(static_cast<char>(value & 0xFF));
    out.put(static_cast<char>((value >> 8) & 0xFF));
}

void m32(std::ostream &out, std::uint32_t value)
{
    m16(out, static_cast<std::uint16_t>(value & 0xFFFF));
    m16(out, static_cast<std::uint16_t>(value >> 16));
}

bool save(const std::vector<std::int16_t> &samples, const std::string &outputPath)
{
    if (samples.size() > (0xFFFFFFFFu - 36u) / 2u)
    {
        return false;
    }

    std::ofstream out(outputPath, std::ios::binary);

    if (!out)
    {
        return false;
    }

    auto size = static_cast<std::uint32_t>(samples.size() * 2);

    out.write("RIFF", 4);
    m32(out, size + 36);
    out.write("WAVE", 4);

    out.write("fmt ", 4);
    m32(out, 16);
    m16(out, 1);
    m16(out, 1);
    m32(out, sampleRate);
    m32(out, sampleRate * 2);
    m16(out, 2);
    m16(out, 16);

    out.write("data", 4);
    m32(out, size);

    for (std::int16_t sample : samples)
    {
        m16(out, static_cast<std::uint16_t>(sample));
    }

    out.close();
    return static_cast<bool>(out);
}

int main(int argc, char *argv[])
{
    Options options;

    try
    {
        options = parse(argc, argv);
    }
    catch (const std::exception &error)
    {
        std::cerr << "Error: " << error.what() << "\n\n";
        help(argv[0]);
        return 1;
    }

    if (options.help || argc == 1)
    {
        help(argv[0]);
        return 0;
    }
    std::ifstream input(options.input, std::ios::binary);

    if (!input)
    {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    std::uint64_t fileSize = 0;
    std::uint32_t crc = 0xFFFFFFFFu;
    char current;

    while (input.get(current))
    {
        crc = checksum(crc, static_cast<unsigned char>(current));
        ++fileSize;
    };

    if (!input.eof())
    {
        std::cerr << "Failed to read input file";
        return 1;
    };

    crc ^= 0xFFFFFFFFu;

    input.clear();
    input.seekg(0, std::ios::beg);

    if (!input)
    {
        std::cerr << "Could not rewind input file\n";
        return 1;
    }

    std::cout << "Input size: " << fileSize << " bytes\n";
    std::cout << "CRC-32: " << std::hex << crc
              << std::dec << '\n';

    std::vector<std::int16_t> quoteunquotemusic;

    auto eByte = [&](unsigned char value)
    {
        unsigned int note1 = value / 16;
        unsigned int note2 = value % 16;

        auto samples1 = generateTone(frequency(note1));
        auto samples2 = generateTone(frequency(note2));

        quoteunquotemusic.insert(
            quoteunquotemusic.end(),
            samples1.begin(),
            samples1.end());

        quoteunquotemusic.insert(
            quoteunquotemusic.end(),
            samples2.begin(),
            samples2.end());
    };

    const char magic[] = "MUS1";

    for (int i = 0; i < 4; ++i)
    {
        eByte(static_cast<unsigned char>(magic[i]));
    }

    for (int i = 0; i < 8; ++i)
    {
        eByte(static_cast<unsigned char>((fileSize >> (8 * i)) & 0xFFu));
    };

    for (int i = 0; i < 4; ++i)
    {
        eByte(static_cast<unsigned char>((crc >> (8 * i)) & 0xFFu));
    }

    char byte;

    while (input.get(byte))
    {
        eByte(static_cast<unsigned char>(byte));
    }

    if (!input.eof())
    {
        std::cerr << "Failed while reading input file\n";
        return 1;
    }

    double length = static_cast<double>(quoteunquotemusic.size()) / sampleRate;
    std::cout << "Length: " << length << " seconds\n";

    if (!save(quoteunquotemusic, options.output))
    {
        std::cerr << "Failed to save file\n";
        return 1;
    }

    std::cout << "Saved " << options.output << "\n";

    return 0;
}