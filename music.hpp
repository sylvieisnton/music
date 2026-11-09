#pragma once

#include <cmath>
#include <stdexcept>

enum class Scale : unsigned char
{
    Chromatic = 0,
    Pentatonic = 1,
    Minor = 2
};

inline double noteFrequency(int note, Scale scale)
{
    if (note < 0 || note > 15) {
        throw std::invalid_argument("Note must be between 0 and 15");
    }

    int semitones;

    switch (scale)
    {
        case Scale::Chromatic:
            semitones = note;
            break;

        case Scale::Pentatonic:
        {
            constexpr int steps[] = {0, 2, 4, 7, 9};
            semitones = (note / 5) * 12 + steps[note % 5];
            break;
        }

        case Scale::Minor:
        {
            constexpr int steps[] = {0, 2, 3, 5, 7, 8, 10};
            semitones = (note / 7) * 12 + steps[note % 7];
            break;
        }

        default:
            throw std::invalid_argument("Unknown scale");
    }

    return 440.0 * std::pow(2.0, semitones / 12.0);
}