#pragma once

#include <string>
#include <string_view>
#include <cstdint>
#include <memory>

typedef struct sqlite3 sqlite3;

class DBStatement;

class DBHandle final
{
public:
    DBHandle (uint32_t pageSize, uint32_t cacheSize);
    ~DBHandle ();

    std::unique_ptr<DBStatement> createStatement (std::string_view stmt) const;

    explicit operator bool () const noexcept;
private:
    std::string mFileName;
    sqlite3* mDB { nullptr };

    bool setupDB (uint32_t pageSize, uint32_t cacheSize);
    void logError ();

    void cleanupFile ();
};