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

constexpr int sampleRate = 44100;
constexpr int samplesPerNote = sampleRate / 10;
constexpr double pi = 3.14159265358979323846;

int identifyNote(const std::vector<double> &samples, std::size_t start, int count, Scale scale)
{
    int best = 0;
    double bestScore = -1.0;

    for (int note = 0; note < 16; ++note)
    {
        double frequency = 440.0 * std::pow(2.0, note / 12.0);

        double sine = 0.0;
        double cosine = 0.0;

        for (int i = 0; i < count; ++i)
        {
            double frequency = noteFrequency(note, scale);
            double angle = 2.0 * pi * frequency * i / sampleRate;
            double sample = samples[start + i];

            sine += sample * std::sin(angle);
            cosine += sample * std::cos(angle);
        }

        double score = sine * sine + cosine * cosine;

        if (score > bestScore)
        {
            bestScore = score;
            best = note;
        }
    }

    return best;
}

bool readNote(std::istream &input, int &note, int count = sampleRate / 10, Scale scale = Scale::Chromatic)
{
    std::vector<double> samples(count);

    for (int i = 0; i < count; ++i)
    {
        int low = input.get();
        int high = input.get();

        if (low == std::char_traits<char>::eof() || high == std::char_traits<char>::eof())
        {
            return false;
        }

        int value = low | (high << 8);

        if (value >= 32768)
        {
            value -= 65536;
        }

        samples[i] = static_cast<double>(value);
    }

    note = identifyNote(samples, 0, count, scale);
    return true;
}

bool readByte(std::istream &input, unsigned char &byte, int count = sampleRate / 10, Scale scale = Scale::Chromatic)
{
    int note1;
    int note2;

    if (!readNote(input, note1, count, scale) || !readNote(input, note2, count, scale))
    {
        return false;
    }

    byte = static_cast<unsigned char>(note1 * 16 + note2);

    return true;
}

struct MusicHeader
{
    std::uint64_t fileSize = 0;
    std::uint32_t crc = 0;
    int noteSamples = sampleRate / 10;
    Scale scale = Scale::Chromatic;
};

bool readMusicHeader(std::istream &input, MusicHeader &header)
{
    unsigned char bytes[18];

    for (int i = 0; i < 18; ++i)
    {
        if (!readByte(input, bytes[i]))
        {
            std::cerr << "Incomplete music header\n";
            return false;
        }
    }

    if (bytes[0] != 'M' ||
        bytes[1] != 'U' ||
        bytes[2] != 'S' ||
        bytes[3] != '3')
    {
        std::cerr << "Unable to find header\n";
        return false;
    }

    header.fileSize = 0;
    header.crc = 0;

    for (int i = 0; i < 8; ++i)
    {
        header.fileSize |= static_cast<std::uint64_t>(bytes[4 + i]) << (8 * i);
    }

    for (int i = 0; i < 4; ++i)
    {
        header.crc |= static_cast<std::uint32_t>(bytes[12 + i]) << (8 * i);
    }

    switch (bytes[16])
    {
    case 0:
        header.noteSamples = sampleRate / 5;
        break;

    case 1:
        header.noteSamples = sampleRate / 10;
        break;

    case 2:
        header.noteSamples = sampleRate / 20;
        break;

    default:
        std::cerr << "Invalid speed ID\n";
        return false;
    }

    if (bytes[17] > 2)
    {
        std::cerr << "Invalid scale ID\n";
        return false;
    }

    header.scale = static_cast<Scale>(bytes[17]);

    return true;
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
        std::cerr << "Failed to open input file." << std::endl;
        return 1;
    }

    char header[44];

    if (!input.read(header, 44))
    {
        std::cerr << "Failed to read WAV header." << std::endl;
        return 1;
    }

    if (std::memcmp(header, "RIFF", 4) != 0 || std::memcmp(header + 8, "WAVE", 4) != 0 || std::memcmp(header + 36, "data", 4) != 0)
    {
        std::cerr << "Invalid WAV file format." << std::endl;
        return 1;
    }

    std::vector<double> samples;

    auto headerNumber = [&](int offset, int count)
    {
        std::uint32_t value = 0;

        for (int i = 0; i < count; ++i)
        {
            value |= static_cast<std::uint32_t>(static_cast<unsigned char>(header[offset + i])) << (8 * i);
        }

        return value;
    };

    if (std::memcmp(header + 12, "fmt ", 4) != 0 ||
        headerNumber(16, 4) != 16 ||
        headerNumber(20, 2) != 1 ||
        headerNumber(22, 2) != 1 ||
        headerNumber(24, 4) != sampleRate ||
        headerNumber(32, 2) != 2 ||
        headerNumber(34, 2) != 16)
    {
        std::cerr << "Unsupported format\n";
        return 1;
    }

    std::uint64_t dataSize = headerNumber(40, 4);

    constexpr std::uint64_t headerAudioBytes = 18ull * 2ull * (sampleRate / 10) * 2ull;

    if (dataSize < headerAudioBytes)
    {
        std::cerr << "Incomplete music header\n";
        return 1;
    }

    MusicHeader music;

    if (!readMusicHeader(input, music))
    {
        return 1;
    }

    std::uint64_t payloadAudioBytes =
        dataSize - headerAudioBytes;

    std::uint64_t audioBytesPerByte =
        2ull * music.noteSamples * 2ull;

    if (payloadAudioBytes % audioBytesPerByte != 0 ||
        music.fileSize != payloadAudioBytes / audioBytesPerByte)
    {
        std::cerr << "File length check failed\n";
        return 1;
    }

    std::error_code pathError;

    if (std::filesystem::equivalent(
            options.input, options.output, pathError))
    {
        std::cerr << "Input and output must be different files\n";
        return 1;
    }

    std::ofstream output(options.output, std::ios::binary);

    if (!output)
    {
        std::cerr << "Could not open output file\n";
        return 1;
    }

    std::uint32_t actualCrc = 0xFFFFFFFFu;

    auto started = std::chrono::steady_clock::now();
    auto lastUpdate = started;

    for (std::uint64_t i = 0; i < music.fileSize; ++i)
    {
        unsigned char byte;

        if (!readByte(input, byte, music.noteSamples, music.scale))
        {
            std::cerr << "Audio ended early\n";
            return 1;
        }

        actualCrc = checksum(actualCrc, byte);
        output.put(static_cast<char>(byte));

        if (!output)
        {
            std::cerr << "Failed while writing output\n";
            return 1;
        }

        auto now = std::chrono::steady_clock::now();
        std::uint64_t processed = i + 1;

        if (now - lastUpdate >= std::chrono::milliseconds(250) ||
            processed == music.fileSize)
        {

            double elapsed = std::chrono::duration<double>(now - started).count();

            double percent = 100.0 * static_cast<double>(processed) / music.fileSize;

            double remaining = elapsed * static_cast<double>(music.fileSize - processed) / processed;

            std::cout << '\r' << std::fixed << std::setprecision(1) << "Decoding: " << percent << "% | " << processed << '/' << music.fileSize << " bytes | ETA: " << remaining << " s          " << std::flush;

            lastUpdate = now;
        }
    }

    if (music.fileSize == 0)
    {
        std::cout << "Decoding: 100% | 0/0 bytes";
    }

    std::cout << '\n'
              << std::defaultfloat;

    output.close();

    if (!output)
    {
        std::cerr << "Failed to finish output file\n";
        return 1;
    }

    actualCrc ^= 0xFFFFFFFFu;

    if (actualCrc != music.crc)
    {
        std::cerr
            << "Checksum mismatch: output file is damaged\n";
        return 1;
    }

    std::cout << "Checksum verified!\n";
    std::cout << "Saved " << options.output << '\n';

    return 0;
}