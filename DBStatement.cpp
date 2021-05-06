#include "DBStatement.h"

#include <sqlite3.h>
#include <fmt/format.h>

DBStatement::DBStatement (sqlite3_stmt* stmt)
    : mStmt(stmt)
{
}

DBStatement::~DBStatement ()
{
    reset ();
    sqlite3_finalize (mStmt);
}

bool DBStatement::bind (int index, int value) const
{
    const int rc = sqlite3_bind_int (mStmt, index, value);

    if (rc != SQLITE_OK)
        fmt::print ("Bind error: {}\n", sqlite3_errstr (rc));

    return rc == SQLITE_OK;
}

bool DBStatement::bind (int index, double value) const
{
    const int rc = sqlite3_bind_double (mStmt, index, value);

    if (rc != SQLITE_OK)
        fmt::print ("Bind error: {}\n", sqlite3_errstr (rc));

    return rc == SQLITE_OK;
}

size_t DBStatement::bind (int index, const DataBuffer& buffer, CompressionMethod method) const
{
    DataBuffer::Result result = buffer.compress (method);

    const int rc = sqlite3_bind_blob (mStmt, index, result.Data, result.Size, result.Free);

    if (rc != SQLITE_OK)
        fmt::print ("Bind error: {}\n", sqlite3_errstr (rc));

    return rc == SQLITE_OK ? result.Size : 0;
}

int DBStatement::exec () const
{
    const int rc = sqlite3_step (mStmt);

    if (rc != SQLITE_OK && rc != SQLITE_ROW && rc != SQLITE_DONE)
        fmt::print ("Exec error: {}\n", sqlite3_errstr (rc));

    return rc;
}

bool DBStatement::reset () const
{
    int rc = sqlite3_clear_bindings (mStmt);

    if (rc != SQLITE_OK)
    {
        fmt::print ("Clear error: {}\n", sqlite3_errstr (rc));
        return false;
    }

    rc = sqlite3_reset (mStmt);

    if (rc != SQLITE_OK)
        fmt::print ("Reset error: {}\n", sqlite3_errstr (rc));

    return rc == SQLITE_OK;
}

