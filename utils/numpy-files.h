
#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <vector>
#include <functional>

// --------------------------------------------------------------------
// --- Taken from IDM/tskit_test/native/numpy-files.cpp
// --------------------------------------------------------------------


namespace Kernel
{

enum dtype {
    int8,
    uint8,
    int16,
    uint16,
    int32,
    uint32,
    int64,
    uint64,
    float32,
    float64
};

struct descr {
    std::string name;
    size_t size;
    bool integer;
    bool is_signed;
};

extern const std::map<dtype, descr> dtypes;

typedef std::function<void(size_t num_bytes, std::ofstream& file)> write_data_func;

// This version allows the user to write the data when the data is not contiguos.
// For example, I have a list of genomes with nucleotide sequences.  I use this version
// to create a file that has the sequenes as a two dimensional array.
void write_numpy_file( write_data_func func,
                       dtype datatype,
                       std::vector<size_t>& dimensions,
                       const std::string& filename,
                       bool isAppending );

void write_numpy_file( void* data,
                       dtype datatype,
                       std::vector<size_t>& dimensions,
                       const std::string& filename );

// NOTE: pdata will have memory created for it using malloc() so you'll need to use free() when releasing it.
void read_numpy_file( const std::string& filename,
                      void*& pdata,
                      dtype& data_type,
                      std::vector<size_t>& dimensions );

struct aligned_buffer {
    void* base;
    size_t stride;  // in bytes
    inline void* row(size_t index) { return (void*) ((char*)base + index*stride); }
};

void read_numpy_file_aligned( const std::string& filename,
                              aligned_buffer& pdata,
                              dtype& data_type,
                              std::vector<size_t>& dimensions,
                              size_t alignment = 32 );

}