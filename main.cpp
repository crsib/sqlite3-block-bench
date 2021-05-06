#include <chrono>
#include <cstdio>
#include <algorithm>

#include <fmt/format.h>

#include <benchmark/benchmark.h>

#include "Block.h"

#include "DBHandle.h"
#include "DBStatement.h"

// 0 - compression method
// 1 - page size
// 2 - cache size in pages


static void BM_Write (benchmark::State& state) 
{
    static std::vector<Block> blocks = Generate (Method::Mixed, 200);

    const CompressionMethod method = CompressionMethod (state.range (0));

    size_t blocksProcessed = 0;
    size_t bytesWritten = 0;

    for (auto _ : state) 
    {
        state.PauseTiming ();

        DBHandle db (state.range (1), state.range (2));

        if (!db)
        {
            state.SkipWithError ("DB setup failed");
            break;
        }

        std::unique_ptr<DBStatement> stmt = db.createStatement (R"=(
        INSERT INTO
            sampleblocks (blockid, sampleformat, summin, summax, sumrms, summary256, summary64k,samples)
            VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8))=");

        if (!stmt)
        {
            state.SkipWithError ("Statement setup failed");
            break;
        }

        state.ResumeTiming ();

        for (const Block& block : blocks)
        {
            size_t written;

            if (0 == (written = WriteBlock (block, *stmt, method)))
            {
                state.SkipWithError ("Write failed");
                break;
            }

            bytesWritten += written;
            ++blocksProcessed;
        }
    }

    state.counters["Total Size"] = bytesWritten;

    state.counters["Blocks"] = blocksProcessed;
    state.counters["Blocks/s"] = benchmark::Counter (blocksProcessed, benchmark::Counter::kIsRate);
}

BENCHMARK (BM_Write)->ArgsProduct ({
    // Compression 
    { 0, 1, 2 },
    // Page sizes
    { 1 << 11, 1 << 12, 1 << 13, 1 << 14, 1 << 15, 1 << 16 },
    // Cache size
    { 128, 256, 512, 1024 },
})->Iterations(4)->MeasureProcessCPUTime ()->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN ();
