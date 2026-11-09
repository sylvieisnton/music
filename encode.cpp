#include "args.hpp"
#include "checksum.hpp"
#include "music.hpp"

#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <chrono>
#include <iomanip>

double frequency(int note)
{
    return 440.0 * std::pow(2.0, (note) / 12.0);
}

constexpr int sampleRate = 44100;
constexpr double pi = 3.14159265358979323846;

std::vector<std::int16_t> generateTone(double frequency, int sampleCt)
{
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

bool writeWavHeader(std::ostream &out, std::uint64_t sampleCt)
{
    if (sampleCt > (0xFFFFFFFFu - 36u) / 2u)
    {
        return false;
    }

    auto dataSize = static_cast<std::uint32_t>(sampleCt * 2);

    out.write("RIFF", 4);
    m32(out, 36u + dataSize);
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
    m32(out, dataSize);
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

    unsigned char speedId = 1;
    int noteSamples = sampleRate / 10;

    if (options.speed == "slow")
    {
        speedId = 0;
        noteSamples = sampleRate / 5;
    }
    else if (options.speed == "fast")
    {
        speedId = 2;
        noteSamples = sampleRate / 20;
    }

    Scale payloadScale = Scale::Chromatic;

    if (options.scale == "pentatonic")
    {
        payloadScale = Scale::Pentatonic;
    }
    else if (options.scale == "minor")
    {
        payloadScale = Scale::Minor;
    }

    constexpr std::uint64_t headerSamples = 18ull * 2ull * (sampleRate / 10);

    std::uint64_t samplesPerByte = 2ull * noteSamples;

    constexpr std::uint64_t maxSamples = (0xFFFFFFFFull - 36ull) / 2ull;

    if (fileSize > (maxSamples - headerSamples) / samplesPerByte)
    {
        std::cerr << "Input is too large for this WAV format\n";
        return 1;
    }

    std::uint64_t totalSamples = headerSamples + fileSize * samplesPerByte;

    std::error_code pathError;

    if (std::filesystem::equivalent(
            options.input, options.output, pathError))
    {
        std::cerr << "Input and output must be different files\n";
        return 1;
    }

    std::ofstream out(options.output, std::ios::binary);

    if (!out)
    {
        std::cerr << "Failed to open output file\n";
        return 1;
    }

    if (!writeWavHeader(out, totalSamples))
    {
        std::cerr << "Failed to write WAV header\n";
        return 1;
    }

    auto eByte = [&](unsigned char value, int count = sampleRate / 10, Scale scale = Scale::Chromatic)
    {
        unsigned int note1 = value / 16;
        unsigned int note2 = value % 16;

        auto samples1 = generateTone(noteFrequency(note1, scale), count);
        auto samples2 = generateTone(noteFrequency(note2, scale), count);

        for (std::int16_t sample : samples1)
        {
            m16(out, static_cast<std::uint16_t>(sample));
        }

        for (std::int16_t sample : samples2)
        {
            m16(out, static_cast<std::uint16_t>(sample));
        }
    };

    const char magic[] = "MUS3";

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

    eByte(speedId);

    eByte(static_cast<unsigned char>(payloadScale));

    char byte;

    std::uint64_t processed = 0;

    auto started = std::chrono::steady_clock::now();
    auto lastUpdate = started;

    while (input.get(byte))
    {
        eByte(static_cast<unsigned char>(byte), noteSamples, payloadScale);

        if (!out)
        {
            std::cerr << "\nFailed while writing audio\n";
            return 1;
        }

        ++processed;

        auto now = std::chrono::steady_clock::now();

        if (now - lastUpdate >= std::chrono::milliseconds(250) ||
            processed == fileSize)
        {

            double elapsed =
                std::chrono::duration<double>(now - started).count();

            double percent =
                100.0 * static_cast<double>(processed) / fileSize;

            double remaining =
                elapsed * static_cast<double>(fileSize - processed) / processed;

            std::cout << '\r'
                      << std::fixed << std::setprecision(1)
                      << "Encoding: " << percent << "% | "
                      << processed << '/' << fileSize << " bytes | "
                      << "ETA: " << remaining << " s          "
                      << std::flush;

            lastUpdate = now;
        }
    }

    if (fileSize == 0)
    {
        std::cout << "Encoding: 100% | 0/0 bytes";
    }

    std::cout << '\n'
              << std::defaultfloat;

    if (!input.eof())
    {
        std::cerr << "Failed while reading input file\n";
        return 1;
    }

    out.close();

    if (!out)
    {
        std::cerr << "Failed to write output file\n";
        return 1;
    }

    double length = static_cast<double>(totalSamples) / sampleRate;

    std::cout << "Length: " << length << " seconds\n";
    std::cout << "Saved " << options.output << '\n';

    return 0;
}