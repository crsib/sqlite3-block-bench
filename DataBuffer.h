#pragma once

#include <cstddef>

enum class CompressionMethod
{
    None,
    Snappy,
    LZ4
};

class DataBuffer
{
public:
    DataBuffer (const void* data, size_t s);
    ~DataBuffer ();

    struct Result
    {
        const void* Data;
        size_t Size;

        void (* Free)(void*);
    };

    Result compress (CompressionMethod method) const;
private:
    Result compressLZ4 () const;
    Result compressSnappy () const;

    const void* mData;
    size_t mSize;
};