#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <chrono>
#include <cstdio>

#include <gflags/gflags.h>
#include <sqlite3.h>
#include <fmt/format.h>
#include <snappy.h>
#include <lz4.h>

DEFINE_string(compression, "off", "off|snappy|lz4");
DEFINE_int32(page_size_exp, 12, "Page size exponent (2^page_size_exp)");
DEFINE_int32(cache_size, 512, "Number of pages before commit");
DEFINE_int32(blocks_count, 500, "Number of pages before commit");
DEFINE_string(method, "mixed", "zero|random|sine");

struct Block final
{
    int BlockId;
    int Format;
    double SumMin;
    double SumMax;
    double SumRMS;

    uint8_t Summary256[12288];
    uint8_t Summary[48];

    float Data[1024 * 1024 / sizeof (float)];

    static constexpr size_t ValuesCount = sizeof (Data) / sizeof (Data[0]);
};

enum class Method
{
    Zero,
    Random,
    Sine,
    Mixed,
};

template<typename Generator>
Block CreateBlock(int blockId, Generator gen)
{
    Block blk = {
            blockId,
            1,
            0.0,
            0.0,
            0.0,
            {},
            {}
    };

    for (size_t i = 0; i < Block::ValuesCount; ++i)
        blk.Data[i] = gen();

    return blk;
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

std::vector<Block> Generate(Method method, size_t blocksCount)
{
    std::vector<Block> output;

    output.reserve (blocksCount);

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

    for (size_t i = 0; i < blocksCount; ++i)
        output.emplace_back(CreateBlock (i + 1, generator));

    return output;
}

Method GetMethod()
{
    if (FLAGS_method == "sine")
        return Method::Sine;
    else if (FLAGS_method == "random")
        return  Method::Random;
    else if (FLAGS_method == "mixed")
        return  Method::Mixed;
    else
        return Method::Zero;
}

struct SQLiteHandle
{
    sqlite3* DB { nullptr };

    SQLiteHandle()
    {
        if (sqlite3_open ("test.sqlite3", &DB))
        {
            std::cout << sqlite3_errmsg (DB) << std::endl;
        }
    }

    ~SQLiteHandle()
    {
        if (DB != nullptr)
        {
            sqlite3_close (DB);
            std::remove ("test.sqlite3");
        }
    }

    bool SetupDB()
    {
        if (sqlite3_exec (DB, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr))
        {
            std::cout << sqlite3_errmsg(DB) << std::endl;
            return false;
        }

        if (sqlite3_exec (DB,
                          fmt::format ("PRAGMA page_size={}", 1 << FLAGS_page_size_exp).c_str(),
                          nullptr, nullptr, nullptr)
                )
        {
            std::cout << sqlite3_errmsg(DB) << std::endl;
            return false;
        }

        if (sqlite3_exec (DB,
                          fmt::format ("PRAGMA cache_size={}", 1 << FLAGS_cache_size).c_str(),
                          nullptr, nullptr, nullptr)
                )
        {
            std::cout << sqlite3_errmsg(DB) << std::endl;
            return false;
        }

        const char* createSTMT = R"=(CREATE TABLE sampleblocks(
        blockid              INTEGER PRIMARY KEY AUTOINCREMENT,
        sampleformat         INTEGER,
        summin               REAL,
        summax               REAL,
        sumrms               REAL,
        summary256           BLOB,
        summary64k           BLOB,
        samples              BLOB))=";

        if (sqlite3_exec (DB, createSTMT, nullptr, nullptr, nullptr))
        {
            std::cout << sqlite3_errmsg(DB) << std::endl;
            return false;
        }

        return true;
    }

    void logError()
    {
        std::cout << sqlite3_errmsg(DB) << std::endl;
    }
};

struct StatementHandle
{
    sqlite3_stmt* Stmt {nullptr};

    StatementHandle (sqlite3* db, const char* sql)
    {
        if (sqlite3_prepare_v2 (db, sql, -1, &Stmt, nullptr))
        {
            std::cout << sqlite3_errmsg(db) << std::endl;
        }
    }

    ~StatementHandle()
    {
        if (Stmt != nullptr)
            sqlite3_finalize (Stmt);
    }

    bool bind(int index, int value) const
    {
        return 0 == sqlite3_bind_int(Stmt, index, value);
    }

    bool bind(int index, double value) const
    {
        return 0 == sqlite3_bind_double(Stmt, index, value);
    }

    bool bind(int index, const void* data, int size) const
    {
        return 0 == sqlite3_bind_blob(Stmt, index, data, size, SQLITE_STATIC);
    }

    int exec () const
    {
        return sqlite3_step (Stmt);
    }

    int reset () const
    {
        return sqlite3_reset (Stmt);
    }
};

enum class CompressionMethod
{
    None,
    Snappy,
    LZ4
};

std::string CompressSnappy(const void* data, size_t size)
{
    std::string result;

    snappy::Compress (reinterpret_cast<const char*>(data), size, &result);

    return result;
}

std::vector<char> CompressLZ4(const void* data, size_t size)
{
    std::vector<char> buffer;
    buffer.resize (LZ4_compressBound(size));

    int bytes = LZ4_compress_default(
        reinterpret_cast<const char*>(data),
        buffer.data(),
        size,
        buffer.size()
    );

    buffer.erase (buffer.begin() + bytes, buffer.end());

    return buffer;
}

bool WriteBlock (const StatementHandle& handle, const Block& block, CompressionMethod method)
{
    handle.reset();

    if (!handle.bind(1, block.BlockId))
        return false;

    if (!handle.bind(2, block.Format))
        return false;

    if (!handle.bind(3, block.SumMin))
        return false;

    if (!handle.bind(4, block.SumMax))
        return false;

    if (!handle.bind(5, block.SumRMS))
        return false;

    if (!handle.bind(6, block.Summary256, sizeof (block.Summary256)))
        return false;

    if (!handle.bind(7, block.Summary, sizeof (block.Summary)))
        return false;

    if (method == CompressionMethod::Snappy)
    {
        std::string compressed = CompressSnappy (block.Data, sizeof (block.Data));

        if (!handle.bind (8, compressed.data(), compressed.size()))
            return false;
    }
    else if (method == CompressionMethod::LZ4)
    {
        std::vector<char> compressed = CompressLZ4(block.Data, sizeof (block.Data));

        if (!handle.bind (8, compressed.data(), compressed.size()))
            return false;
    }
    else
    {
        if (!handle.bind (8, block.Data, sizeof (block.Data)))
            return false;
    }

    return handle.exec() == SQLITE_DONE;
}

CompressionMethod GetCompressionMethod()
{
    if (FLAGS_compression == "snappy")
        return CompressionMethod::Snappy;
    else if (FLAGS_compression == "lz4")
        return CompressionMethod::LZ4;
    else
        return CompressionMethod::None;
}

int main (int argc, char** argv)
{
    gflags::ParseCommandLineFlags (&argc, &argv, true);

    std::cout << "Comp: " <<
        FLAGS_compression << " Meth: " <<
        FLAGS_method << " Cache: " <<
        FLAGS_cache_size << " Page: " <<
      (1 << FLAGS_page_size_exp) << std::endl;

    std::vector<Block> blocks = Generate (GetMethod (), FLAGS_blocks_count);

    SQLiteHandle handle;

    if (handle.DB == nullptr)
        return 1;

    if (!handle.SetupDB ())
        return 2;

    StatementHandle writeStatement(handle.DB, R"=(INSERT INTO
        sampleblocks (blockid, sampleformat,summin, summax, sumrms, summary256, summary64k,samples)
        VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8))="
    );

    if (writeStatement.Stmt == nullptr)
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

    std::cout << "\tWrite: " << writeDuration << " " <<   std::endl;

    fmt::print ("Write: {} s, {} block/s\n", writeDuration, FLAGS_blocks_count / writeDuration);

    return 0;
}
