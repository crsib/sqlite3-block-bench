#include "Block.h"

#include <random>
#include <algorithm>
#include <cmath>
#include <sqlite3.h>

#include "DBStatement.h"

namespace
{
template<typename Generator>
void CreateBlock(Block& blk, int blockId, Generator gen)
{
    blk.BlockId = blockId;

    for (size_t i = 0; i < Block::ValuesCount; ++i)
        blk.Data[i] = gen();
}

float NextZero()
{
    return 0;
}

float NextRandom()
{
    static std::random_device r;
    static std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
    static std::mt19937 engine(seed);
    static std::uniform_real_distribution<float> distrib(-1.0f, 1.0f);

    return distrib (engine);
}

float NextSine()
{
    static int64_t i = 0;

    return float (std::sin (M_PI_2 * i / 1000.0)) + (std::sin (M_PI_2 * i / 2000.0)) + (std::sin (M_PI_2 * i / 3000.0));
}

float NextMixed()
{
    static std::random_device r;
    static std::seed_seq seed{r(), r(), r(), r(), r(), r(), r(), r()};
    static std::mt19937 engine(seed);
    static std::uniform_int_distribution<> distrib(0, 100);

    const int value = distrib(engine);

    if (value < 20)
        return NextZero();
    else if (value < 70 )
        return NextSine();
    else
        return  NextRandom();
}
}

std::vector<Block> Generate(Method method, size_t blocksCount)
{
    std::vector<Block> output;

    output.resize (blocksCount);

    float (*generator)();

    switch (method)
    {
        case Method::Zero:
            generator = NextZero;
            break;
        case Method::Random:
            generator = NextRandom;
            break;
        case Method::Sine:
            generator = NextSine;
            break;
        case Method::Mixed:
            generator = NextMixed;
            break;
    }

    std::for_each (output.begin (), output.end (), [generator, beg = &output[0]] (Block& blk) {
        CreateBlock (blk, std::distance(beg, &blk) + 1, generator);
    });

    return output;
}

size_t WriteBlock (const Block& block, const DBStatement& stmt, CompressionMethod method)
{
    if (!stmt.reset ())
        return 0;

    if (!stmt.bind (1, block.BlockId))
        return 0;

    if (!stmt.bind (2, block.Format))
        return 0;

    if (!stmt.bind (3, block.SumMin))
        return 0;

    if (!stmt.bind (4, block.SumMax))
        return 0;

    if (!stmt.bind (5, block.SumRMS))
        return 0;

    if (!stmt.bind (6, DataBuffer (block.Summary256, sizeof (block.Summary256)), CompressionMethod::None))
        return 0;

    if (!stmt.bind (7, DataBuffer (block.Summary, sizeof (block.Summary)), CompressionMethod::None))
        return 0;

    const size_t bytesWritten = stmt.bind (8, DataBuffer (block.Data, sizeof (block.Data)), method);

    if (!bytesWritten)
        return 0;

    return stmt.exec () == SQLITE_DONE ? bytesWritten : 0;
}

