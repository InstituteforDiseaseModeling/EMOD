
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "UnitTest++.h"
#include "FileSystem.h"
#include "componentTests.h"

#include "numpy-files.h"

using namespace Kernel; 

SUITE( numpyfilesTest )
{
    TEST( TestReadAndWrite )
    {
        int32_t expected_data[ 2 ][ 6 ] = {
            {1,0,1,0,0,1},
            {0,1,0,1,1,0}
        };

        // -------------------------------------------------------------------------------------------------
        // --- Read data from a file that was produced with componentTests\testdata\numpy-filesTest\test.py
        // --- Verify that it has the data we expect.
        // -------------------------------------------------------------------------------------------------
        void* p_read_data = nullptr;
        dtype data_type = dtype::int8;
        std::vector<size_t> dimensions;

        read_numpy_file( "testdata/numpy-filesTest/numpy-filesTest.npy", p_read_data, data_type, dimensions );

        CHECK_EQUAL( dtype::int32, data_type );
        CHECK_EQUAL( 2, dimensions.size() );
        CHECK_EQUAL( 2, dimensions[ 0 ] );
        CHECK_EQUAL( 6, dimensions[ 1 ] );

        int32_t* p_actual_data = (int32_t*)p_read_data;

        for( int i = 0; i < 2; ++i )
        {
            for( int j = 0; j < 6; ++j )
            {
                CHECK_EQUAL( expected_data[ i ][ j ], *(p_actual_data + (6 * i + j)) );
            }
        }

        free( p_read_data );
        p_read_data = nullptr;

        // ----------------------------------------------------------------------------
        // --- Write the expected data to a file and verify that we can read it back in
        // --- and still get the correct data.
        // ----------------------------------------------------------------------------

        write_numpy_file( expected_data, dtype::int32, dimensions, "testdata/numpy-filesTest/tmp.npy" );

        void* p_read_data2 = nullptr;
        dtype data_type2 = dtype::int8;
        std::vector<size_t> dimensions2;

        read_numpy_file( "testdata/numpy-filesTest/tmp.npy", p_read_data2, data_type2, dimensions2 );

        CHECK_EQUAL( dtype::int32, data_type2 );
        CHECK_EQUAL( 2, dimensions2.size() );
        CHECK_EQUAL( 2, dimensions2[ 0 ] );
        CHECK_EQUAL( 6, dimensions2[ 1 ] );

        int32_t* p_actual_data2 = (int32_t*)p_read_data2;

        for( int i = 0; i < 2; ++i )
        {
            for( int j = 0; j < 6; ++j )
            {
                CHECK_EQUAL( expected_data[ i ][ j ], *(p_actual_data2 + (6 * i + j)) );
            }
        }

        free( p_read_data2 );
        p_read_data2 = nullptr;

        // -------------------------------
        // --- remove this temporary file
        // -------------------------------
        bool success = FileSystem::RemoveFile( "testdata/numpy-filesTest/tmp.npy" );

        CHECK( success );
    }

    TEST( TestReadAndWriteWithFunc )
    {
        int32_t expected_data_A[ 10 ] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        int32_t expected_data_B[ 10 ] = { 9, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
        int32_t expected_data_C[ 10 ] = { 8, 9, 0, 1, 2, 3, 4, 5, 6, 7 };
        int32_t expected_data_D[ 10 ] = { 7, 8, 9, 0, 1, 2, 3, 4, 5, 6 };
        int32_t expected_data_E[ 10 ] = { 6, 7, 8, 9, 0, 1, 2, 3, 4, 5 };


        write_data_func my_func = [expected_data_A,
            expected_data_B,
            expected_data_C,
            expected_data_D,
            expected_data_E](size_t num_bytes, std::ofstream& file)
        {
            size_t size_of_X = sizeof( int32_t ) * 10;
            size_t expected_num_bytes = size_of_X * 5;
            file.write( (const char*)expected_data_A, size_of_X );
            file.write( (const char*)expected_data_B, size_of_X );
            file.write( (const char*)expected_data_C, size_of_X );
            file.write( (const char*)expected_data_D, size_of_X );
            file.write( (const char*)expected_data_E, size_of_X );

            CHECK_EQUAL( expected_num_bytes, num_bytes );
        };

        dtype write_data_type = dtype::int32;
        std::vector<size_t> write_dimensions;
        write_dimensions.push_back( 5 );
        write_dimensions.push_back( 10 );

        std::string temp_filename = "testdata/numpy-filesTest/TestReadAndWriteWithFunc_junk_.npy";

        write_numpy_file( my_func, write_data_type, write_dimensions, temp_filename, false );

        // -------------------------------------------------------------------------------------------------
        // --- Read data from a file that was produced with componentTests\testdata\numpy-filesTest\test.py
        // --- Verify that it has the data we expect.
        // -------------------------------------------------------------------------------------------------
        void* p_read_data = nullptr;
        dtype read_data_type = dtype::int8;
        std::vector<size_t> read_dimensions;

        read_numpy_file( temp_filename, p_read_data, read_data_type, read_dimensions );

        CHECK_EQUAL( dtype::int32, read_data_type );
        CHECK_EQUAL( dtype::int32, write_data_type );
        CHECK_EQUAL( 2, read_dimensions.size() );
        CHECK_EQUAL( 5, read_dimensions[ 0 ] );
        CHECK_EQUAL( 10, read_dimensions[ 1 ] );

        int32_t* p_actual_data = (int32_t*)p_read_data;

        for( int j = 0; j < 10; ++j )
        {
            CHECK_EQUAL( expected_data_A[ j ], *(p_actual_data + (10 * 0 + j)) );
            CHECK_EQUAL( expected_data_B[ j ], *(p_actual_data + (10 * 1 + j)) );
            CHECK_EQUAL( expected_data_C[ j ], *(p_actual_data + (10 * 2 + j)) );
            CHECK_EQUAL( expected_data_D[ j ], *(p_actual_data + (10 * 3 + j)) );
            CHECK_EQUAL( expected_data_E[ j ], *(p_actual_data + (10 * 4 + j)) );
        }

        free( p_read_data );
        p_read_data = nullptr;

        // -------------------------------
        // --- remove this temporary file
        // -------------------------------
        bool success = FileSystem::RemoveFile( temp_filename );

        CHECK( success );
    }

    TEST( TestReadAndWriteWithAppending )
    {
        int32_t expected_data_A[ 10 ] = { 0, 6, 7, 8, 9, 1, 2, 3, 4, 5 };
        int32_t expected_data_B[ 10 ] = { 9, 5, 6, 7, 8, 0, 1, 2, 3, 4 };
        int32_t expected_data_C[ 10 ] = { 8, 4, 5, 6, 7, 9, 0, 1, 2, 3 };
        int32_t expected_data_D[ 10 ] = { 7, 3, 4, 5, 6, 8, 9, 0, 1, 2 };
        int32_t expected_data_E[ 10 ] = { 6, 2, 3, 4, 5, 7, 8, 9, 0, 1 };

        write_data_func my_func_AB = [expected_data_A,
                                      expected_data_B ](size_t num_bytes, std::ofstream& file)
        {
            size_t size_of_X = sizeof( int32_t ) * 10;
            size_t expected_num_bytes = size_of_X * 2;
            file.write( (const char*)expected_data_A, size_of_X );
            file.write( (const char*)expected_data_B, size_of_X );

            CHECK_EQUAL( expected_num_bytes, num_bytes );
        };

        write_data_func my_func_C = [expected_data_C](size_t num_bytes, std::ofstream& file)
        {
            size_t size_of_X = sizeof( int32_t ) * 10;
            size_t expected_num_bytes = size_of_X * 1;
            file.write( (const char*)expected_data_C, size_of_X );

            CHECK_EQUAL( expected_num_bytes, num_bytes );
        };

        write_data_func my_func_DE = [expected_data_D,
                                      expected_data_E ](size_t num_bytes, std::ofstream& file)
        {
            size_t size_of_X = sizeof( int32_t ) * 10;
            size_t expected_num_bytes = size_of_X * 2;
            file.write( (const char*)expected_data_D, size_of_X );
            file.write( (const char*)expected_data_E, size_of_X );

            CHECK_EQUAL( expected_num_bytes, num_bytes );
        };

        std::string temp_filename = "testdata/numpy-filesTest/TestReadAndWriteWithAppending_junk_.npy";
        dtype write_data_type = dtype::int32;
        std::vector<size_t> write_dimensions;
        write_dimensions.push_back( 2 );
        write_dimensions.push_back( 10 );

        write_numpy_file( my_func_AB, write_data_type, write_dimensions, temp_filename, false );

        write_dimensions.clear();
        write_dimensions.push_back( 1 );
        write_dimensions.push_back( 10 );

        write_numpy_file( my_func_C, write_data_type, write_dimensions, temp_filename, true );

        write_dimensions.clear();
        write_dimensions.push_back( 2 );
        write_dimensions.push_back( 10 );

        write_numpy_file( my_func_DE, write_data_type, write_dimensions, temp_filename, true );

        // -------------------------------------------------------------------------------------------------
        // --- Read data from a file that was produced with componentTests\testdata\numpy-filesTest\test.py
        // --- Verify that it has the data we expect.
        // -------------------------------------------------------------------------------------------------
        void* p_read_data = nullptr;
        dtype read_data_type = dtype::int8;
        std::vector<size_t> read_dimensions;

        read_numpy_file( temp_filename, p_read_data, read_data_type, read_dimensions );

        CHECK_EQUAL( dtype::int32, read_data_type );
        CHECK_EQUAL( dtype::int32, write_data_type );
        CHECK_EQUAL( 2, read_dimensions.size() );
        CHECK_EQUAL( 5, read_dimensions[ 0 ] );
        CHECK_EQUAL( 10, read_dimensions[ 1 ] );

        int32_t* p_actual_data = (int32_t*)p_read_data;

        for( int j = 0; j < 10; ++j )
        {
            CHECK_EQUAL( expected_data_A[ j ], *(p_actual_data + (10 * 0 + j)) );
            CHECK_EQUAL( expected_data_B[ j ], *(p_actual_data + (10 * 1 + j)) );
            CHECK_EQUAL( expected_data_C[ j ], *(p_actual_data + (10 * 2 + j)) );
            CHECK_EQUAL( expected_data_D[ j ], *(p_actual_data + (10 * 3 + j)) );
            CHECK_EQUAL( expected_data_E[ j ], *(p_actual_data + (10 * 4 + j)) );
        }

        free( p_read_data );
        p_read_data = nullptr;

        // -------------------------------
        // --- remove this temporary file
        // -------------------------------
        bool success = FileSystem::RemoveFile( temp_filename );

        CHECK( success );
    }
}