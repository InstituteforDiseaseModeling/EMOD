

// --------------------------------------------------------------------
// --- Taken from IDM/tskit_test/native/numpy-files.cpp
// --------------------------------------------------------------------

#include "stdafx.h"
#include "numpy-files.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>

#include "FileSystem.h"
#include "Exceptions.h"
#include "Log.h"

SETUP_LOGGING( "numpy-files" )

#ifdef WIN32
#define posix_memalign(p, a, s) (((*(p)) = _aligned_malloc((s), (a))), *(p) ?0 :errno)
#endif

namespace Kernel
{
const std::map<dtype, descr> dtypes = {
    { int8,    { "int8",    1, true,  true  } },
    { uint8,   { "uint8",   1, true,  false } },
    { int16,   { "int16",   2, true,  true  } },
    { uint16,  { "uint16",  2, true,  false } },
    { int32,   { "int32",   4, true,  true  } },
    { uint32,  { "uint32",  4, true,  false } },
    { int64,   { "int64",   8, true,  true  } },
    { uint64,  { "uint64",  8, true,  false } },
    { float32, { "float32", 4, false, true  } },
    { float64, { "float64", 8, false, true  } }
};

static void _write_magic_number(std::ofstream& file);
static void _write_version(std::ofstream& file);
static void _build_header(dtype datatype, std::vector<size_t>& dimensions, std::ostringstream& hdr, size_t& num_bytes);
static void _write_header(std::string& header, std::ofstream& file);
static void _write_data(void *data, size_t num_bytes, std::ofstream& file);

static void _write_start_file( std::ofstream& file,
                               size_t& num_bytes,
                               dtype datatype,
                               std::vector<size_t>& dimensions,
                               const std::string& filename );
static void _write_close_file( std::ofstream& file,
                               dtype datatype,
                               size_t num_bytes,
                               const std::string& filename );
static void _check_can_append( const std::string& filename,
                               dtype existing_datatype,
                               const std::vector<size_t>& existing_dimensions,
                               dtype datatype,
                               const std::vector<size_t>& dimensions );
static void _append_data_to_file( const std::string& filename,
                               dtype datatype,
                               std::vector<size_t>& existing_dimensions,
                               std::vector<size_t>& dimensions,
                               write_data_func func );

static void _check_magic_number(std::ifstream& file, const std::string& filename);
static size_t _check_version(std::ifstream& file, const std::string& filename);
static size_t _get_header_size(std::ifstream& file, size_t version);
static void _check_fortran_order( const std::string& filename, const std::string& header );
static dtype _decode_descr( const std::string& filename, const std::string& header );
static void _decode_shape(const std::string& header, std::vector<size_t>& dimensions );
static void _read_and_process_header( std::ifstream& file,
                                      const std::string& filename,
                                      dtype& datatype,
                                      std::vector<size_t>& dimensions );

void write_numpy_file( write_data_func func,
                       dtype datatype,
                       std::vector<size_t>& dimensions,
                       const std::string& filename,
                       bool isAppending)
{
    if( isAppending )
    {
        // --------------------------------------------------------------------
        // --- Get data type and dimensions of the arrays in the existing file
        // --------------------------------------------------------------------
        std::ifstream file;
        FileSystem::OpenFileForReading( file, filename.c_str(), true );

        dtype existing_datatype = dtype::int32;
        std::vector<size_t> existing_dimensions;
        _read_and_process_header( file, filename, existing_datatype, existing_dimensions );

        file.close();

        // -------------------------------------------------------------
        // --- Check that data in the file matches what you want to add
        // --- and then add the data
        // -------------------------------------------------------------
        _check_can_append( filename, existing_datatype, existing_dimensions, datatype, dimensions );

        _append_data_to_file( filename, datatype, existing_dimensions, dimensions, func );
    }
    else
    {
        std::ofstream file;
        size_t num_bytes = 0;

        _write_start_file( file, num_bytes, datatype, dimensions, filename );
        func( num_bytes, file );

        _write_close_file( file, datatype, num_bytes, filename );
    }
}

void write_numpy_file( void* data, dtype datatype, std::vector<size_t>& dimensions, const std::string& filename )
{
    std::ofstream file;
    size_t num_bytes = 0;

    _write_start_file( file, num_bytes, datatype, dimensions, filename );

    _write_data( data, num_bytes, file );

    _write_close_file( file, datatype, num_bytes, filename );
}

void _check_can_append( const std::string& filename,
                        dtype existing_datatype,
                        const std::vector<size_t>& existing_dimensions,
                        dtype datatype,
                        const std::vector<size_t>& dimensions )
{
    if( existing_datatype != datatype )
    {
        std::stringstream ss;
        ss << "Error appending to " << filename << "\n";
        ss << "Existing numpy array file is of type " << existing_datatype << " but you want to append data of type " << datatype;
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }
    if( existing_dimensions.size() != dimensions.size() )
    {
        std::stringstream ss;
        ss << "Error appending to " << filename << "\n";
        ss << "Existing numpy array is not the same shape as the new one.\n";
        ss << "Existing shape has " << existing_dimensions.size() << " dimensions.\n";
        ss << "New shape has " << dimensions.size() << " dimensions.\n";
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }
    for( int i = 1; i < existing_dimensions.size(); ++i )
    {
        if( existing_dimensions[ i ] != dimensions[ i ] )
        {
            std::stringstream ss;
            ss << "Error appending to " << filename << "\n";
            ss << "Existing numpy array has different sizes of the inner arrays than the new one.\n";
            ss << "Existing shape is: ";
            for( int j = 0; j < existing_dimensions.size(); ++j )
            {
                ss << existing_dimensions[ j ];
                if( (j + 1) < existing_dimensions.size() )
                {
                    ss << " x ";
                }
            }
            ss << "New shape is: ";
            for( int j = 0; j < dimensions.size(); ++j )
            {
                ss << dimensions[ j ];
                if( (j + 1) < dimensions.size() )
                {
                    ss << " x ";
                }
            }
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }
}

void _append_data_to_file( const std::string& filename,
                           dtype datatype,
                           std::vector<size_t>& existing_dimensions,
                           std::vector<size_t>& dimensions,
                           write_data_func func )
{
    // ---------------------------------------------------------------
    // --- Determine the number of bytes to add to the end of the file
    // ---------------------------------------------------------------
    descr description = dtypes.at(datatype);
    size_t num_bytes = description.size;
    for (size_t i = 0; i < dimensions.size(); ++i)
    {
        num_bytes *= dimensions[i];
    }

    // ---------------------------------------------------------------------------------------------
    // --- Open the file for input/output.  This allows to move the pointer around in the file
    // --- and won't delete the contents of the file.
    // ---    std::ios_base::out - By itself, it deletes the contents
    // ---    std::ios_base::app - By itself, keeps contents, but you can only add to the end of the file.
    // ---    std::ios_base::in | std::ios_base::out - keeps the contents, allows writing in the middle, and appending to the end.
    // ---------------------------------------------------------------------------------------------
    std::ofstream appending_file;
    appending_file.open( filename.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary );

    // write the new data to the end of the file
    appending_file.seekp( 0, std::ios::end );
    func( num_bytes, appending_file );

    // create string with the number of outer array elements
    size_t num_elements = existing_dimensions[ 0 ] + dimensions[ 0 ];
    char num_elements_buff[15];
    sprintf_s( num_elements_buff, "%12zd", num_elements ); // keep the same number of characters for this dimension

                                                           // 60 is the spot in the header that the numbef of outer array elements starts at
    appending_file.seekp( 60, std::ios::beg );
    appending_file.write( num_elements_buff, strlen( num_elements_buff ) );

    _write_close_file( appending_file, datatype, num_bytes, filename );
}

void _write_start_file( std::ofstream& file,
                        size_t& num_bytes,
                        dtype datatype,
                        std::vector<size_t>& dimensions,
                        const std::string& filename )
{
    FileSystem::OpenFileForWriting( file, filename.c_str(), true, false );

    _write_magic_number(file);
    _write_version(file);

    std::ostringstream header;
    _build_header(datatype, dimensions, header, num_bytes);

    std::string header_str = header.str();
    _write_header(header_str, file);
}

void _write_close_file( std::ofstream& file,
                        dtype datatype,
                        size_t num_bytes,
                        const std::string& filename )
{
    file.close();
    if( file.good() ) 
    {
        LOG_DEBUG_F( "Wrote %zd bytes (%zd elements) to '%s'.", num_bytes,(num_bytes / dtypes.at(datatype).size), filename.c_str());
    } else 
    {
        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), "file.good() returned false" );
    }
}

void _write_magic_number(std::ofstream& file)
{
    char magic[] = { char(0x93), 'N', 'U', 'M', 'P', 'Y'};
    file.write(magic, 6);

    return;
}

void _write_version(std::ofstream& file)
{
    uint8_t major = 1;
    uint8_t minor = 0;

    file.write((char*)&major, 1);
    file.write((char*)&minor, 1);

    return;
}

void _build_header(dtype datatype, std::vector<size_t>& dimensions, std::ostringstream& hdr, size_t& num_bytes)
{
    descr description = dtypes.at(datatype);

    // ----------------------------------------------------------------------------------------
    // --- I'm putting the 12 spaces into this dimension so that we can change this value later
    // --- if we are appending.  The spaces allow us to write a larger number without changing
    // --- the size of the total header.
    // ----------------------------------------------------------------------------------------
    char num_elements_buff[15];
    sprintf_s( num_elements_buff, "%12zd", dimensions[ 0 ] );

    hdr << "{'descr': '"
        << (description.integer ? (description.is_signed ? 'i' : 'u') : 'f')
        << description.size
        << "', 'fortran_order': False, 'shape': ("
        << num_elements_buff
        << ", ";

    num_bytes = description.size;
    num_bytes *= dimensions[0];
    for (size_t i = 1; i < dimensions.size(); ++i)
    {
        hdr << dimensions[i];
        if( (i+1) < dimensions.size() )
        {
            hdr << ", ";
        }
        num_bytes *= dimensions[i];
    }
    hdr << "), }\n";
    for (size_t count = 6 + 2 + 2 + hdr.str().size(); count % 64 != 0; ++count)
    {
        hdr << ' ';
    }

    return;
}

void _write_header(std::string& header, std::ofstream& file)
{
    uint16_t header_len = uint16_t(header.size());

    file.write((char*)&header_len, 2);
    file.write((char*)header.data(), header.size());

    return;
}

void _write_data(void *data, size_t num_bytes, std::ofstream& file)
{
    file.write((char *)data, num_bytes);

    return;
}

void read_numpy_file(const std::string& filename, void*& pdata, dtype& datatype, std::vector<size_t>& dimensions)
{
    std::ifstream file;
    FileSystem::OpenFileForReading( file, filename.c_str(), true );

    _read_and_process_header( file, filename, datatype, dimensions );

    size_t bytes = dtypes.at(datatype).size;
    for (auto dimension : dimensions) 
    {
        bytes *= dimension;
    }

    pdata = malloc(bytes);
    file.read((char*)pdata, bytes);

    file.close();
}

void _read_and_process_header( std::ifstream& file,
                               const std::string& filename,
                               dtype& datatype,
                               std::vector<size_t>& dimensions )
{
    _check_magic_number(file, filename);
    size_t version = _check_version(file, filename);
    size_t header_size = _get_header_size(file, version);
    // {'descr':'u4','fortran_order':False,'shape':(<num_nodes>,<num_intervals>)}
    std::string header(header_size, ' ');
    file.read((char*)header.data(), header_size);

    _check_fortran_order( filename, header );
    datatype = _decode_descr( filename, header );
    _decode_shape( header, dimensions );
}

void _check_magic_number(std::ifstream& file, const std::string& filename)
{
    char buffer[6];
    file.read(buffer, 6);
    char magic[] = { char(0x93), 'N', 'U', 'M', 'P', 'Y'};
    if ( memcmp(buffer, magic, 6) != 0 ) 
    {
        std::stringstream ss;
        ss << "'magic' number for " << filename << " does not match NumPy magic number '\\x93NUMPY'";
        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
    }
}

size_t _check_version(std::ifstream& file, const std::string& filename)
{
    uint8_t major;
    file.read((char*)&major, 1);
    if( (major != 1) && (major != 2) ) 
    {
        std::stringstream ss;
        ss << "Unsupported NumPy file version - " << major << " in file '" << filename << "'.";
        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
    }

    uint8_t minor;
    file.read((char*)&minor, 1);

    return size_t(major);
}

size_t _get_header_size(std::ifstream& file, size_t version)
{
    size_t header_size;
    if ( version == 1 ) 
    {
        uint16_t temp;
        file.read((char*)&temp, sizeof(temp));
        header_size = size_t(temp);
    }
    else 
    {
        uint32_t temp;
        file.read((char*)&temp, sizeof(temp));
        header_size = size_t(temp);
    }

    return header_size;
}

// ensure 'fortran_order':False
void _check_fortran_order( const std::string& filename, const std::string& header )
{
    // ¡hacky code! - assumes that if "False" is in the header, it is part of 'fortran_order'
    auto False = header.find("False");
    if ( False == std::string::npos ) 
    {
        std::stringstream ss;
        ss << "Did not find 'fortran_order':False in file header ('" << header << "')";;
        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
    }
}

// decode descr, '[<=]?[iuf][1248]' supported
dtype _decode_descr( const std::string& filename, const std::string& header )
{
    auto descr = header.find("descr");
    auto colon = header.find(":", descr);
    auto open = header.find("'", colon);
    auto close = header.find("'", open + 1);
    ++open;
    std::string format = header.substr(open, close-open);

    std::stringstream msg;
    msg << "decode_descr() parsing descr '" << format << "'..." << std::endl;
    LOG_DEBUG_F("%s",msg.str().c_str());
    // https://numpy.org/doc/stable/reference/generated/numpy.dtype.byteorder.html
    switch (format[0]) {
        case '<':   // little-endian (Intel x86)
        case '=':   // machine native format
        case '|':   // "not applicable"
            format = format.substr(1);
            break;
        case '>':   // big-endian (Motorla, PowerPC)
        {
            std::stringstream ss;
            ss << "NumPy file is big-endian which is currently unsupported ('" << header << "')";
            throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
        }
        default:    // no byte-order mark, apparently
            break;
    }
    dtype datatype;
    switch (format[0]) {
        case 'i':   // (signed) integer
            switch (format[1]) {
                case '1':
                    datatype = int8;
                    break;
                case '2':
                    datatype = int16;
                    break;
                case '4':
                    datatype = int32;
                    break;
                case '8':
                    datatype = int64;
                    break;
                default:
                    {
                        std::stringstream ss;
                        ss << "Unknown or unsupported data size in NumPy file ('" << header << "')";
                        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
                    }
            }
            break;
        case 'u':   // unsigned integer
            switch (format[1]) {
                case '1':
                    datatype = uint8;
                    break;
                case '2':
                    datatype = uint16;
                    break;
                case '4':
                    datatype = uint32;
                    break;
                case '8':
                    datatype = uint64;
                    break;
                default:
                    {
                        std::stringstream ss;
                        ss << "Unknown or unsupported data size in NumPy file ('" << header << "')";
                        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
                    }
            }
            break;
        case 'f':   // floating point
            switch (format[1]) {
                case '4':
                    datatype = float32;
                    break;
                case '8':
                    datatype = float64;
                    break;
                default:
                    {
                        std::stringstream ss;
                        ss << "Unknown or unsupported data size in NumPy file ('" << header << "')";
                        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
                    }
            }
            break;
        default:
            {
                std::stringstream ss;
                ss << "Unknown data type for NumPy file ('" << header << "')";
                throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
            }
            exit(1);
    }

    LOG_DEBUG_F("decode_descr() returning '%s'.", dtypes.at(datatype).name.c_str() );

    return datatype;
}

// decode shape -> std::vector<size_t>
void _decode_shape(const std::string& header, std::vector<size_t>& dimensions)
{
    auto shape = header.find("shape");
    auto open = header.find("(", shape);
    ++open;
    auto close = header.find(")", open);

    std::list<size_t> separators;
    for ( auto comma = header.find(",", open); comma < close; comma = header.find(",", comma+1) ) 
    {
        separators.push_back(comma);
    }
    separators.push_back(close);

    std::stringstream ss;
    ss << "header = '" << header;
    ss << "decode_shape() starting at " << open << ", separators at: ";
    for ( auto entry : separators ) 
    {
        ss << entry << " ";
    }
    ss << "\n";

    dimensions.clear();
    for ( size_t start = open, finish = separators.front();
          start < close;
          start = finish+1, separators.pop_front(), finish = separators.front()) 
    {
        ss << "decode_shape() parsing '" << header.substr(start, finish - start) << "'" << std::endl;
        dimensions.push_back(std::stoull(header.substr(start, finish - start)));
        if( separators.size() == 1 ) break;
    }

    ss << "decode_shape() found the following dimensions: ";
    for ( auto entry : dimensions ) {
        ss << entry << " ";
    }
    ss << "\n";
    LOG_DEBUG_F("%s",ss.str().c_str());
}

void read_numpy_file_aligned(const std::string& filename, aligned_buffer& data, dtype& datatype, std::vector<size_t>& dimensions, size_t alignment)
{
    data.base = 0;
    data.stride = 0;

    std::ifstream file;
    FileSystem::OpenFileForReading( file, filename.c_str(), true );

    _check_magic_number(file, filename);
    size_t version = _check_version(file, filename);
    size_t header_size = _get_header_size(file, version);

    std::string header(header_size, ' ');
    file.read((char*)header.data(), header_size);

    _check_fortran_order( filename, header );
    datatype = _decode_descr( filename, header );
    _decode_shape(header, dimensions);

    size_t element_size = dtypes.at(datatype).size;

    size_t bytes_per_vector = dimensions[dimensions.size()-1] * element_size;   // raw bytes per vector
    size_t stride = (bytes_per_vector + alignment - 1) & ~(alignment - 1);      // aligned bytes per vector

    size_t total_bytes = stride;
    size_t total_rows = 1;
    for (size_t idim = 0; idim < dimensions.size() - 1; ++idim ) 
    {
        size_t dimension = dimensions[idim];
        total_rows *= dimension;
    }
    total_bytes *= total_rows;

    // data.base = aligned_alloc(alignment, total_bytes);
    if (posix_memalign(&data.base, alignment, total_bytes) == 0) 
    {
        data.stride = stride;

        for ( size_t irow = 0; irow < total_rows; ++irow ) 
        {
            file.read((char*)(data.row(irow)), bytes_per_vector);
        }
    }
    else
    {
        std::stringstream ss;
        ss << "Could not allocate " << total_bytes << " aligned to " << alignment << " byte boundary." << std::endl;
        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
    }

    file.close();
}

} // end namespace