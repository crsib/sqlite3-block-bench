#pragma once

#include "DataBuffer.h"

typedef struct sqlite3_stmt sqlite3_stmt;

class DBStatement final
{
    friend class DBHandle;

    explicit DBStatement (sqlite3_stmt* stmt);
public:
    ~DBStatement ();

    bool bind (int index, int value) const;
    bool bind (int index, double value) const;
    size_t bind (int index, const DataBuffer& buffer, CompressionMethod method) const;

    int exec () const;

    bool reset () const;
private:
    sqlite3_stmt* mStmt { nullptr };
};