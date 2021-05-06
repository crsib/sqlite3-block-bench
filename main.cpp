#include <chrono>
#include <cstdio>
#include <algorithm>

#include <fmt/format.h>

#include <benchmark/benchmark.h>

#include "Block.h"

#include "DBHandle.h"
#include "DBStatement.h"

// 0 - page size
// 1 - cache size in pages
// 2 - compression method

static void BM_Write (benchmark::State& state) 
{
    static std::vector<Block> blocks = Generate (Method::Mixed, 200);

    size_t blocksProcessed = 0;
    size_t bytesWritten = 0;

    for (auto _ : state) 
    {
        state.PauseTiming ();

        DBHandle db (state.range (0), state.range (1));

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

            if (0 == (written = WriteBlock (block, *stmt, CompressionMethod (state.range (2)))))
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
    // Page sizes
    { 1 << 11, 1 << 12, 1 << 13, 1 << 14, 1 << 15, 1 << 16 },
    // Cache size
    { 128, 256, 512, 1024 },
    // Compression 
    { 0, 1, 2 }
})->MinTime(2);

BENCHMARK_MAIN ();

/*
int main (int argc, char** argv)
{
    gflags::ParseCommandLineFlags (&argc, &argv, true);

    fmt::print (
        "Method: {}, Compression: {}, Page size: {}, Cache size: {}\n", 
        FLAGS_method, 
        FLAGS_compression, 
        (1 << FLAGS_page_size_exp), 
        FLAGS_cache_size
    );

    std::vector<Block> blocks = Generate (GetMethod (), FLAGS_blocks_count);

    SQLiteHandle handle;

    if (handle.DB == nullptr)
        return 1;

    if (!handle.SetupDB ())
        return 2;

    DBStatement writeStatement(handle.DB, R"=(INSERT INTO
        sampleblocks (blockid, sampleformat,summin, summax, sumrms, summary256, summary64k,samples)
        VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8))="
    );

    if (writeStatement.mStmt == nullptr)
    {
        handle.logError ();
        return 3;
    }

    const std::chrono::high_resolution_clock::time_point startWrite =
            std::chrono::high_resolution_clock::now ();

    for (const Block& blk : blocks)
    {
        if (!WriteBlock (writeStatement, blk, GetCompressionMethod()))
        {
            handle.logError ();
            return 4;
        }
    }

    const std::chrono::high_resolution_clock::time_point endWrite =
            std::chrono::high_resolution_clock::now ();

    const double writeDuration = std::chrono::duration<double> (endWrite - startWrite).count();

    fmt::print ("Write: {} s, {} block/s\n", writeDuration, FLAGS_blocks_count / writeDuration);

    return 0;
}
*/