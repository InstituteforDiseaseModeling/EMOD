/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "componentTests.h"
#include "GenomeMarkers.h"

using namespace std;
using namespace Kernel;

SUITE( GenomeMarkersTest )
{
    TEST( TestInitialize )
    {
        std::vector<std::string> names;
        names.push_back( "Marker1" );
        names.push_back( "Marker2" );
        names.push_back( "Marker3" );
        names.push_back( "Marker4" );
        names.push_back( "Marker5" );

        GenomeMarkers gm;
        int num_substrains = gm.Initialize( names );
        CHECK_EQUAL( 32, num_substrains );

        CHECK_EQUAL( 5, gm.Size() );
        const std::set<std::string>& r_name_set = gm.GetNameSet();
        for( auto& name_in_vector : names )
        {
            CHECK( r_name_set.find( name_in_vector ) != r_name_set.end() );
        }
        CHECK_EQUAL(  1, gm.GetBits( "Marker1" ) );
        CHECK_EQUAL(  2, gm.GetBits( "Marker2" ) );
        CHECK_EQUAL(  4, gm.GetBits( "Marker3" ) );
        CHECK_EQUAL(  8, gm.GetBits( "Marker4" ) );
        CHECK_EQUAL( 16, gm.GetBits( "Marker5" ) );

        uint32_t bits = gm.CreateBits( names );
        CHECK_EQUAL( 31, bits );

        names.pop_back();
        bits = gm.CreateBits( names );
        CHECK_EQUAL( 15, bits );

        names.pop_back();
        bits = gm.CreateBits( names );
        CHECK_EQUAL( 7, bits );

        names.pop_back();
        bits = gm.CreateBits( names );
        CHECK_EQUAL( 3, bits );

        names.pop_back();
        bits = gm.CreateBits( names );
        CHECK_EQUAL( 1, bits );

        names.pop_back();
        bits = gm.CreateBits( names );
        CHECK_EQUAL( 0, bits );
    }

    void TestHelper_InitializeException( int lineNumber, const std::vector<std::string>& rNames, const std::string& rExpMsg )
    {
        try
        {
            GenomeMarkers gm;

            gm.Initialize( rNames );

            CHECK_LN( false, lineNumber ); // should not get here
        }
        catch( DetailedException& re )
        {
            std::string msg = re.GetMsg();
            if( msg.find( rExpMsg ) == string::npos )
            {
                PrintDebug( rExpMsg );
                PrintDebug( msg );
                CHECK_LN( false, lineNumber );
            }
        }
    }

    TEST( TestDuplicateName )
    {
        std::vector<std::string> names;
        names.push_back( "Marker1" );
        names.push_back( "Marker"  );
        names.push_back( "Marker3" );
        names.push_back( "marker"  );
        names.push_back( "Marker5" );

        TestHelper_InitializeException( __LINE__, names,
                                        "Duplicate name in Genome_Markers = 'marker'.  Each name must be case-insensitive unique." );
    }

    TEST( TestTooManyMarkers )
    {
        std::vector<std::string> names;
        names.push_back( "Marker1" );
        names.push_back( "Marker2" );
        names.push_back( "Marker3" );
        names.push_back( "Marker4" );
        names.push_back( "Marker5" );
        names.push_back( "Marker6" );
        names.push_back( "Marker7" );
        names.push_back( "Marker8" );
        names.push_back( "Marker9" );
        names.push_back( "Marker10" );
        names.push_back( "Marker11" );
        names.push_back( "Marker12" );
        names.push_back( "Marker13" );
        names.push_back( "Marker14" );
        names.push_back( "Marker15" );
        names.push_back( "Marker16" );
        names.push_back( "Marker17" );
        names.push_back( "Marker18" );
        names.push_back( "Marker19" );
        names.push_back( "Marker20" );
        names.push_back( "Marker21" );
        names.push_back( "Marker22" );
        names.push_back( "Marker23" );
        names.push_back( "Marker24" );
        names.push_back( "Marker25" );
        names.push_back( "Marker26" );
        names.push_back( "Marker27" );
        names.push_back( "Marker28" );
        names.push_back( "Marker29" );
        names.push_back( "Marker30" );
        names.push_back( "Marker31" );
        names.push_back( "Marker32" );
        names.push_back( "Marker33" );

        TestHelper_InitializeException( __LINE__, names,
                                        "33 genome markers have been defined.  The maximum is 32." );
    }

    TEST( TestCreateBitsUnknownMarker )
    {
        std::vector<std::string> names;
        names.push_back( "Marker1" );
        names.push_back( "Marker2" );
        names.push_back( "Marker3" );
        names.push_back( "Marker4" );
        names.push_back( "Marker5" );

        std::vector<std::string> names_in_intervention;
        names_in_intervention.push_back( "Marker5" );
        names_in_intervention.push_back( "Marker1" );
        names_in_intervention.push_back( "My_Bad_Marker" );
        names_in_intervention.push_back( "Marker5" );

        GenomeMarkers gm;
        gm.Initialize( names );

        try
        {
            gm.CreateBits( names_in_intervention );
        }
        catch( DetailedException& re )
        {
            std::string exp_msp("Unknown genome marker name = 'My_Bad_Marker'.  Known markers are:\n'Marker1'...'Marker2'...'Marker3'...'Marker4'...'Marker5'...");
            std::string msg = re.GetMsg();
            if( msg.find( exp_msp ) == string::npos )
            {
                PrintDebug( exp_msp );
                PrintDebug( msg );
                CHECK_LN( false, __LINE__ );
            }

        }
    }

    TEST( TestGetCombinations )
    {
        std::vector<std::string> names;
        names.push_back( "A" );

        std::vector<std::vector<std::string>> combos = GenomeMarkers::GetCombinations( names );

        CHECK_EQUAL( 1, combos.size() );
        CHECK_EQUAL( 1, combos[0].size() );
        CHECK_EQUAL( std::string( "A" ), combos[ 0 ][0] );

        names.push_back( "B" );

        combos = GenomeMarkers::GetCombinations( names );

        CHECK_EQUAL( 3, combos.size() );
        CHECK_EQUAL( 1, combos[ 0 ].size() );
        CHECK_EQUAL( 1, combos[ 1 ].size() );
        CHECK_EQUAL( 2, combos[ 2 ].size() );
        CHECK_EQUAL( std::string( "A" ), combos[ 0 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 1 ][ 0 ] );
        CHECK_EQUAL( std::string( "A" ), combos[ 2 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 2 ][ 1 ] );

        names.push_back( "C" );

        combos = GenomeMarkers::GetCombinations( names );

        CHECK_EQUAL( 7, combos.size() );
        CHECK_EQUAL( 1, combos[ 0 ].size() );
        CHECK_EQUAL( 1, combos[ 1 ].size() );
        CHECK_EQUAL( 1, combos[ 2 ].size() );
        CHECK_EQUAL( 2, combos[ 3 ].size() );
        CHECK_EQUAL( 2, combos[ 4 ].size() );
        CHECK_EQUAL( 2, combos[ 5 ].size() );
        CHECK_EQUAL( 3, combos[ 6 ].size() );

        CHECK_EQUAL( std::string( "A" ), combos[ 0 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 1 ][ 0 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 2 ][ 0 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 3 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 3 ][ 1 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 4 ][ 0 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 4 ][ 1 ] );

        CHECK_EQUAL( std::string( "B" ), combos[ 5 ][ 0 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 5 ][ 1 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 6 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 6 ][ 1 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 6 ][ 2 ] );

        names.push_back( "D" );

        combos = GenomeMarkers::GetCombinations( names );

        CHECK_EQUAL( 15, combos.size() );
        CHECK_EQUAL( std::string( "A" ), combos[ 0 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 1 ][ 0 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 2 ][ 0 ] );
        CHECK_EQUAL( std::string( "D" ), combos[ 3 ][ 0 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 4 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 4 ][ 1 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 5 ][ 0 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 5 ][ 1 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 6 ][ 0 ] );
        CHECK_EQUAL( std::string( "D" ), combos[ 6 ][ 1 ] );

        CHECK_EQUAL( std::string( "B" ), combos[ 7 ][ 0 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 7 ][ 1 ] );

        CHECK_EQUAL( std::string( "B" ), combos[ 8 ][ 0 ] );
        CHECK_EQUAL( std::string( "D" ), combos[ 8 ][ 1 ] );

        CHECK_EQUAL( std::string( "C" ), combos[ 9 ][ 0 ] );
        CHECK_EQUAL( std::string( "D" ), combos[ 9 ][ 1 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 10 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 10 ][ 1 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 10 ][ 2 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 11 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 11 ][ 1 ] );
        CHECK_EQUAL( std::string( "D" ), combos[ 11 ][ 2 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 12 ][ 0 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 12 ][ 1 ] );
        CHECK_EQUAL( std::string( "D" ), combos[ 12 ][ 2 ] );

        CHECK_EQUAL( std::string( "B" ), combos[ 13 ][ 0 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 13 ][ 1 ] );
        CHECK_EQUAL( std::string( "D" ), combos[ 13 ][ 2 ] );

        CHECK_EQUAL( std::string( "A" ), combos[ 14 ][ 0 ] );
        CHECK_EQUAL( std::string( "B" ), combos[ 14 ][ 1 ] );
        CHECK_EQUAL( std::string( "C" ), combos[ 14 ][ 2 ] );
        CHECK_EQUAL( std::string( "D" ), combos[ 14 ][ 3 ] );

        names.push_back( "E" );
        combos = GenomeMarkers::GetCombinations( names );
        CHECK_EQUAL( 31, combos.size() );

        names.push_back( "F" );
        combos = GenomeMarkers::GetCombinations( names );
        CHECK_EQUAL( 63, combos.size() );

#if 0
        names.push_back( "G" );
        names.push_back( "H" );
        names.push_back( "I" );
        names.push_back( "J" );
        names.push_back( "K" );
        names.push_back( "L" );
        names.push_back( "M" );
        names.push_back( "N" );
        names.push_back( "O" );
        names.push_back( "P" );
        names.push_back( "Q" );
        names.push_back( "R" );
        names.push_back( "S" );
        names.push_back( "T" );
        names.push_back( "U" );
        names.push_back( "V" );
        names.push_back( "W" );
        names.push_back( "X" );

        // This is a good test, but it took 42 seconds on my computer
        combos = GenomeMarkers::GetCombinations( names );
        CHECK_EQUAL( 16777215, combos.size() ); // (2^24) - 1
#endif
    }

    TEST( TestCreatePossibleCombinations )
    {
        std::vector<std::string> names;
        names.push_back( "A" );
        names.push_back( "B" );
        names.push_back( "C" );
        names.push_back( "D" );

        GenomeMarkers gm;
        int num_substrains = gm.Initialize( names );
        CHECK_EQUAL( 16, num_substrains );

        std::vector<std::pair<std::string,uint32_t>> combos = gm.CreatePossibleCombinations();

        CHECK_EQUAL( 16, combos.size() );
        CHECK_EQUAL( std::string( "NoMarkers"), combos[  0 ].first );
        CHECK_EQUAL( std::string( "A"        ), combos[  1 ].first );
        CHECK_EQUAL( std::string( "B"        ), combos[  2 ].first );
        CHECK_EQUAL( std::string( "C"        ), combos[  3 ].first );
        CHECK_EQUAL( std::string( "D"        ), combos[  4 ].first );
        CHECK_EQUAL( std::string( "A-B"      ), combos[  5 ].first );
        CHECK_EQUAL( std::string( "A-C"      ), combos[  6 ].first );
        CHECK_EQUAL( std::string( "A-D"      ), combos[  7 ].first );
        CHECK_EQUAL( std::string( "B-C"      ), combos[  8 ].first );
        CHECK_EQUAL( std::string( "B-D"      ), combos[  9 ].first );
        CHECK_EQUAL( std::string( "C-D"      ), combos[ 10 ].first );
        CHECK_EQUAL( std::string( "A-B-C"    ), combos[ 11 ].first );
        CHECK_EQUAL( std::string( "A-B-D"    ), combos[ 12 ].first );
        CHECK_EQUAL( std::string( "A-C-D"    ), combos[ 13 ].first );
        CHECK_EQUAL( std::string( "B-C-D"    ), combos[ 14 ].first );
        CHECK_EQUAL( std::string( "A-B-C-D"  ), combos[ 15 ].first );
        CHECK_EQUAL(  0, combos[  0 ].second );
        CHECK_EQUAL(  1, combos[  1 ].second );
        CHECK_EQUAL(  2, combos[  2 ].second );
        CHECK_EQUAL(  4, combos[  3 ].second );
        CHECK_EQUAL(  8, combos[  4 ].second );
        CHECK_EQUAL(  3, combos[  5 ].second );
        CHECK_EQUAL(  5, combos[  6 ].second );
        CHECK_EQUAL(  9, combos[  7 ].second );
        CHECK_EQUAL(  6, combos[  8 ].second );
        CHECK_EQUAL( 10, combos[  9 ].second );
        CHECK_EQUAL( 12, combos[ 10 ].second );
        CHECK_EQUAL(  7, combos[ 11 ].second );
        CHECK_EQUAL( 11, combos[ 12 ].second );
        CHECK_EQUAL( 13, combos[ 13 ].second );
        CHECK_EQUAL( 14, combos[ 14 ].second );
        CHECK_EQUAL( 15, combos[ 15 ].second );
    }

}
