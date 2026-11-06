#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstdint>

constexpr int sampleRate = 44100;
constexpr int samplesPerNote = sampleRate / 10;
constexpr double pi = 3.14159265358979323846;

int identifyNote(const std::vector<double> &samples, std::size_t start)
{
    int best = 0;
    double bestScore = -1.0;

    for (int note = 0; note < 16; ++note)
    {
        double frequency = 440.0 * std::pow(2.0, note / 12.0);

        double sine = 0.0;
        double cosine = 0.0;

        for (int i = 0; i < samplesPerNote; ++i)
        {
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

int main()
{
    std::ifstream input("song.wav", std::ios::binary);
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

    while (true)
    {
        int low = input.get();
        if (low == std::char_traits<char>::eof())
        {
            break;
        }

        int high = input.get();

        if (high == std::char_traits<char>::eof())
        {
            std::cerr << "Unexpected end of file." << std::endl;
            return 1;
        }

        int value = low | (high << 8);

        if (value >= 32768)
        {
            value -= 65536;
        }

        samples.push_back(static_cast<double>(value));
    }

    if (samples.size() % (samplesPerNote * 2) != 0)
    {
        std::cerr << "Unexpected number of samples." << std::endl;
        return 1;
    }

    std::cout << samples.size() << " samples\n";
    std::cout << "Found " << samples.size() / samplesPerNote << " notes\n";

    std::ofstream output("rec.txt", std::ios::binary);

    if (!output)
    {
        std::cerr << "Failed to open output file." << std::endl;
        return 1;
    }

    for (std::size_t start = 0;
         start < samples.size();
         start += samplesPerNote * 2)
    {

        int firstNote = identifyNote(samples, start);
        int secondNote = identifyNote(samples, start + samplesPerNote);

        int byte = firstNote * 16 + secondNote;
        output.put(static_cast<char>(byte));
    }

    output.close();

    if (!output)
    {
        std::cerr << "Failed to write output file." << std::endl;
        return 1;
    }

    std::cout << "Saved rec.txt" << std::endl;

    return 0;
}