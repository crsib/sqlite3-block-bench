# sqlite3-block-bench

A small benchmark to assess the performance of SQLite3 with the large blocks of data

## Build requirements

* Conan
* CMake
* C++17 compiler

## Arguments

* `--compression`: sets the block compression algorithm. Possible values are **off**, **snappy**, **lz4**. Default: **off**.
* `--page_size_exp`: Exponent of the page size, i.e. `page_size == 2^page_size_exp`. Default: **12** (page size of 4096).
* `--cache_size`: Number of pages to store in the cache. Default value: 512.
* `--blocks_count`: Number of blocks to write. Default value: 500.
* `--generattion_method`: Method to generate the data in the block. Possible values are: **zero**, **sine**, **random**, **mixed**. Default value: **mixed**

