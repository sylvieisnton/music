#include <iostream>
#include <fstream>
#include <cmath>
#include <vector>
#include <cstdint>

double frequency(int note) {
    return 440.0 * std::pow(2.0, (note) / 12.0);
}

constexpr int sampleRate = 44100;
constexpr double pi = 3.14159265358979323846;

std::vector<std::int16_t> generateTone(double frequency) {
    constexpr int sampleCt = sampleRate / 10;
    std::vector<std::int16_t> samples(sampleCt);

    for (int i = 0; i < sampleCt; ++i) {
        double time = static_cast<double>(i) / sampleRate;
        double wave = std::sin(2.0 * pi * frequency * time);
        double envelope = std::sin(pi * i / (sampleCt - 1));
        envelope = envelope * envelope;

        samples[i] = static_cast<std::int16_t>(
            wave * envelope * 8000.0
        );
    }
    return samples;
}

void m16(std::ostream& out, std::int16_t value) {
    out.put(static_cast<char>(value & 0xFF));
    out.put(static_cast<char>((value >> 8) & 0xFF));
}

void m32(std::ostream& out, std::int32_t value) {
    m16(out, static_cast<std::uint16_t>(value & 0xFFFF));
    m16(out, static_cast<std::uint16_t>(value >> 16));
}

bool save(const std::vector<std::int16_t>& samples) {
    if (samples.size() > (0xFFFFFFFFu - 36u)/2u) {
        return false;
    }

    std::ofstream out("song.wav", std::ios::binary);

    if (!out) {
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

    for (std::int16_t sample : samples) {
        m16(out, static_cast<std::uint16_t>(sample));
    }

    out.close();
    return static_cast<bool>(out);
}

int main() {
    std::ifstream input("thing.txt", std::ios::binary);

    if (!input) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    std::vector<std::int16_t> quoteunquotemusic;

    char byte;

    while (input.get(byte)) {
        unsigned int value = static_cast<unsigned char>(byte);
        unsigned int note1 = value / 16;
        unsigned int note2 = value % 16;
        double freq1 = frequency(note1);
        double freq2 = frequency(note2);
        auto samples1 = generateTone(freq1);
        auto samples2 = generateTone(freq2);

        quoteunquotemusic.insert(quoteunquotemusic.end(), samples1.begin(), samples1.end());
        quoteunquotemusic.insert(quoteunquotemusic.end(), samples2.begin(), samples2.end());
    }
    
    double length = static_cast<double>(quoteunquotemusic.size()) / sampleRate;
    std::cout << "Length: " << length << " seconds\n";

    if (!save(quoteunquotemusic)) {
        std::cerr << "Failed to save file\n";
        return 1;
    }

    std::cout << "Saved song.wav\n";

    return 0;
}