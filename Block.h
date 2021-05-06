#pragma once

#include <vector>
#include <cstdint>

#include "DataBuffer.h"

class DBStatement;

enum class Method
{
    Zero,
    Random,
    Sine,
    Mixed,
};

struct Block final
{
    int BlockId;
    int Format {};
    double SumMin {};
    double SumMax {};
    double SumRMS {};

    uint8_t Summary256[12288] {};
    uint8_t Summary[48] {};

    float Data[1024 * 1024 / sizeof (float)];

    static constexpr size_t ValuesCount = sizeof (Data) / sizeof (Data[0]);
};

std::vector<Block> Generate(Method method, size_t blocksCount);

size_t WriteBlock (const Block& blk, const DBStatement& stmt, CompressionMethod method);
