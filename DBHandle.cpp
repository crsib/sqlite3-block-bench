#include "DBHandle.h"

#include <fmt/format.h>
#include <sqlite3.h>
#include <filesystem>

#include "DBStatement.h"

DBHandle::DBHandle (uint32_t pageSize, uint32_t cacheSize)
{
    mFileName = fmt::format ("db_{}_{}.sqlite3", pageSize, cacheSize);

    cleanupFile ();

    if (sqlite3_open (mFileName.c_str (), &mDB))
    {
        fmt::print ("SQLite error: {}", sqlite3_errmsg (mDB));
        return;
    }

    if (!setupDB (pageSize, cacheSize))
    {
        sqlite3_close (mDB);

        cleanupFile ();

        mDB = nullptr;
    }
}

DBHandle::~DBHandle ()
{
    if (mDB != nullptr)
    {
        sqlite3_close (mDB);
        cleanupFile ();
    }
}

std::unique_ptr<DBStatement> DBHandle::createStatement (std::string_view stmtSql) const
{
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2 (mDB, stmtSql.data (), stmtSql.size (), &stmt, nullptr))
    {
        fmt::print ("SQLite error: {}", sqlite3_errmsg (mDB));

        return {};
    }

    return std::unique_ptr<DBStatement> (new DBStatement (stmt));
}

DBHandle::operator bool () const noexcept
{
    return mDB != nullptr;
}

bool DBHandle::setupDB (uint32_t pageSize, uint32_t cacheSize)
{
    if (sqlite3_exec (mDB, "PRAGMA journal_mode=WAL", nullptr, nullptr, nullptr))
    {
        logError ();
        return false;
    }

    if (sqlite3_exec (mDB,
        fmt::format ("PRAGMA page_size={}", pageSize).c_str (),
        nullptr, nullptr, nullptr)
    )
    {
        logError ();
        return false;
    }

    if (sqlite3_exec (mDB,
        fmt::format ("PRAGMA cache_size={}", cacheSize).c_str (),
        nullptr, nullptr, nullptr)
    )
    {
        logError ();
        return false;
    }

    static const char* createSTMT = R"=(
    CREATE TABLE sampleblocks(
        blockid              INTEGER PRIMARY KEY AUTOINCREMENT,
        sampleformat         INTEGER,
        summin               REAL,
        summax               REAL,
        sumrms               REAL,
        summary256           BLOB,
        summary64k           BLOB,
        samples              BLOB
    )
    )=";

    if (sqlite3_exec (mDB, createSTMT, nullptr, nullptr, nullptr))
    {
        logError ();
        return false;
    }

    return true;
}

void DBHandle::logError ()
{
    fmt::print ("SQLite error: {}", sqlite3_errmsg (mDB));
}

void DBHandle::cleanupFile ()
{
    if (std::filesystem::exists (mFileName))
        std::filesystem::remove (mFileName);
}
