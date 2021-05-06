#include "DataBuffer.h"

#include <lz4.h>
#include <snappy.h>

DataBuffer::DataBuffer (const void* data, size_t size)
    : mData(data),
    mSize(size)
{

}

DataBuffer::~DataBuffer ()
{
}

DataBuffer::Result DataBuffer::compress (CompressionMethod method) const
{
    if (method == CompressionMethod::None)
        return { mData, mSize, nullptr };
    else if (method == CompressionMethod::LZ4)
        return compressLZ4 ();
    else if (method == CompressionMethod::Snappy)
        return compressSnappy ();

    return { nullptr, 0, nullptr };
}

DataBuffer::Result DataBuffer::compressLZ4 () const
{
    Result result;

    const int bufferSize = LZ4_compressBound (mSize);
    char* buffer = reinterpret_cast<char*>(::malloc (bufferSize));

    result.Data = buffer;
    result.Free = ::free;

    result.Size = LZ4_compress_default (
        reinterpret_cast<const char*>(mData),
        buffer,
        mSize,
        bufferSize
    );

    return result;
}

DataBuffer::Result DataBuffer::compressSnappy () const
{
    Result result;

    const int bufferSize = snappy::MaxCompressedLength (mSize);

    char* buffer = reinterpret_cast<char*>(::malloc (bufferSize));

    result.Data = buffer;
    result.Free = ::free;

    snappy::RawCompress (
        reinterpret_cast<const char*>(mData),
        mSize,
        buffer,
        &result.Size
    );

    return result;
}
