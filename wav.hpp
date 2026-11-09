#pragma once

#include <cstdint>
#include <cstring>
#include <istream>
#include <stdexcept>

class WavReader
{
public:
    std::uint32_t sampleRate = 0;
    std::uint16_t channels = 0;
    std::uint64_t frameCount = 0;

    explicit WavReader(std::istream& input)
        : input_(input)
    {
        input_.seekg(0, std::ios::end);
        auto end = input_.tellg();

        if (end == std::streampos(-1)) {
            throw std::runtime_error("Cannot determine WAV size");
        }

        auto fileSize = static_cast<std::uint64_t>(
            static_cast<std::streamoff>(end)
        );

        seek(0);

        char id[4];
        readExact(id, 4);

        if (std::memcmp(id, "RIFF", 4) != 0) {
            throw std::runtime_error("Expected WAV audio");
        }

        std::uint64_t riffEnd = 8ull + read32();

        readExact(id, 4);

        if (std::memcmp(id, "WAVE", 4) != 0) {
            throw std::runtime_error("Expected WAVE audio");
        }

        if (riffEnd < 12 || riffEnd > fileSize) {
            throw std::runtime_error("Invalid chunk");
        }

        bool foundFormat = false;
        bool foundData = false;

        std::uint16_t format = 0;
        std::uint16_t bits = 0;
        std::uint16_t alignment = 0;
        std::uint32_t byteRate = 0;

        std::uint64_t dataStart = 0;
        std::uint64_t dataSize = 0;
        std::uint64_t position = 12;

        while (position + 8 <= riffEnd)
        {
            seek(position);

            readExact(id, 4);
            std::uint32_t chunkSize = read32();

            std::uint64_t chunkStart = position + 8;
            std::uint64_t next =
                chunkStart + chunkSize + (chunkSize & 1u);

            if (next > riffEnd) {
                throw std::runtime_error("Invalid chunk");
            }

            if (std::memcmp(id, "fmt ", 4) == 0 &&
                !foundFormat)
            {
                if (chunkSize < 16) {
                    throw std::runtime_error("Invalid chunk");
                }

                format = read16();
                channels = read16();
                sampleRate = read32();
                byteRate = read32();
                alignment = read16();
                bits = read16();

                foundFormat = true;
            }
            else if (std::memcmp(id, "data", 4) == 0 &&
                     !foundData)
            {
                dataStart = chunkStart;
                dataSize = chunkSize;
                foundData = true;
            }

            if (foundFormat && foundData) {
                break;
            }

            position = next;
        }

        if (!foundFormat || !foundData) {
            throw std::runtime_error("Missing WAV format or audio data");
        }

        if (format != 1 || bits != 16) {
            throw std::runtime_error(
                "Unsupported WAV: use standard 16 bit"
            );
        }

        if (channels != 1 && channels != 2) {
            throw std::runtime_error(
                "Unsupported WAV: use mono or stereo"
            );
        }

        if (sampleRate < 8000 || sampleRate > 192000) {
            throw std::runtime_error(
                "Supported sample rates are 8000 to 192000 Hz"
            );
        }

        if (alignment != channels * 2 ||
            byteRate !=
                static_cast<std::uint64_t>(sampleRate) * alignment) {
            throw std::runtime_error("Invalid WAV audio layout");
        }

        if (dataSize % alignment != 0) {
            throw std::runtime_error("Incomplete WAV audio frame");
        }

        frameCount = dataSize / alignment;
        remaining_ = frameCount;

        seek(dataStart);
    }

    std::uint64_t remainingFrames() const
    {
        return remaining_;
    }

    bool readFrame(double& sample)
    {
        if (remaining_ == 0) {
            return false;
        }

        double sum = 0.0;

        for (unsigned int channel = 0;
             channel < channels;
             ++channel)
        {
            int value = read16();

            if (value >= 32768) {
                value -= 65536;
            }

            sum += static_cast<double>(value) / 32768.0;
        }

        sample = sum / channels;
        --remaining_;

        return true;
    }

private:
    std::istream& input_;
    std::uint64_t remaining_ = 0;

    void readExact(char* destination, std::streamsize size)
    {
        if (!input_.read(destination, size)) {
            throw std::runtime_error("Unexpected end of WAV file");
        }
    }

    void seek(std::uint64_t position)
    {
        input_.clear();
        input_.seekg(
            static_cast<std::streamoff>(position),
            std::ios::beg
        );

        if (!input_) {
            throw std::runtime_error("Cannot seek in WAV file");
        }
    }

    std::uint16_t read16()
    {
        unsigned char bytes[2];

        readExact(reinterpret_cast<char*>(bytes), 2);

        return static_cast<std::uint16_t>(
            static_cast<unsigned int>(bytes[0]) |
            (static_cast<unsigned int>(bytes[1]) << 8)
        );
    }

    std::uint32_t read32()
    {
        unsigned char bytes[4];

        readExact(reinterpret_cast<char*>(bytes), 4);

        return static_cast<std::uint32_t>(bytes[0]) |
               (static_cast<std::uint32_t>(bytes[1]) << 8) |
               (static_cast<std::uint32_t>(bytes[2]) << 16) |
               (static_cast<std::uint32_t>(bytes[3]) << 24);
    }
};