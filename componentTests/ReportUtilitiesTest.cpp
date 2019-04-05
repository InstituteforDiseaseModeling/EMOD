/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "componentTests.h"

#include "Exceptions.h"
#include "JsonObject.h"
#include "Serializer.h"
#include "ReportUtilities.h"

using namespace Kernel;

SUITE( ReportUtilitiesTest )
{
    static int m_NextId = 1;

    struct ReportUtilitiesFixture
    {
        ReportUtilitiesFixture()
        {
        }

        ~ReportUtilitiesFixture()
        {
        }
    };

    void InitVector( size_t size3, std::vector<double>& vec )
    {
        for( int i = 0; i < size3; ++i )
        {
            vec.push_back( 0.0 );
        }
    }

    void InitVector( size_t size2, size_t size3, std::vector<std::vector<double>>& vec )
    {
        for( int i = 0; i < size2; ++i )
        {
            vec.push_back( std::vector<double>() );
            InitVector( size3, vec[ i ]);
        }
    }

    void InitVector( size_t size1, size_t size2, size_t size3, std::vector<std::vector<std::vector<double>>>& vec )
    {
        for( int i = 0; i < size1; ++i )
        {
            vec.push_back( std::vector<std::vector<double>>() );
            InitVector( size2, size3, vec[ i ] );
        }
    }

    TEST_FIXTURE( ReportUtilitiesFixture, TestSerializeDeserialize )
    {
        // --------------------
        // --- Initialize test
        // --------------------
        std::string str_oneD = "1-D-Vector";
        std::vector<double> oneD_a;
        oneD_a.push_back( 1.0 );
        oneD_a.push_back( 2.0 );
        oneD_a.push_back( 3.0 );
        oneD_a.push_back( 4.0 );
        oneD_a.push_back( 5.0 );

        std::vector<double> oneD_b;
        oneD_b.push_back( 11.0 );
        oneD_b.push_back( 12.0 );
        oneD_b.push_back( 13.0 );
        oneD_b.push_back( 14.0 );
        oneD_b.push_back( 15.0 );

        std::vector<double> oneD_c;
        oneD_c.push_back( 21.0 );
        oneD_c.push_back( 22.0 );
        oneD_c.push_back( 23.0 );
        oneD_c.push_back( 24.0 );
        oneD_c.push_back( 25.0 );

        std::string str_twoD = "2-D-Vector";
        std::vector<std::vector<double>> twoD_a;
        twoD_a.push_back( oneD_a );
        twoD_a.push_back( oneD_b );
        twoD_a.push_back( oneD_c );

        std::vector<std::vector<double>> twoD_b;
        twoD_b.push_back( oneD_b );
        twoD_b.push_back( oneD_c );
        twoD_b.push_back( oneD_a );

        std::vector<std::vector<double>> twoD_c;
        twoD_c.push_back( oneD_c );
        twoD_c.push_back( oneD_a );
        twoD_c.push_back( oneD_b );

        std::string str_threeD = "3-D-Vector";
        std::vector<std::vector<std::vector<double>>> threeD;
        threeD.push_back( twoD_a );
        threeD.push_back( twoD_b );
        threeD.push_back( twoD_c );

        try
        {
            // -------------------------------
            // --- Seriaize data to a string
            // -------------------------------
            IJsonObjectAdapter* p_serialize = CreateJsonObjAdapter();
            p_serialize->CreateNewWriter();
            p_serialize->BeginObject();

            JSerializer js;

            ReportUtilities::SerializeVector( *p_serialize, js, str_oneD.c_str(), oneD_a );
            ReportUtilities::SerializeVector( *p_serialize, js, str_twoD.c_str(), twoD_a );
            ReportUtilities::SerializeVector( *p_serialize, js, str_threeD.c_str(), threeD );

            p_serialize->EndObject();

            std::string json_data = p_serialize->ToPrettyString();
            //PrintDebug( json_data );

            delete p_serialize;
            p_serialize = nullptr;

            // ---------------------
            // --- Deserialize data
            // ---------------------
            std::vector<double> read_oneD;
            std::vector<std::vector<double>> read_twoD;
            std::vector<std::vector<std::vector<double>>> read_threeD;

            InitVector( oneD_a.size(), read_oneD );
            InitVector( twoD_a.size(), twoD_a[0].size(), read_twoD );
            InitVector( threeD.size(), threeD[0].size(), threeD[0][0].size(), read_threeD );

            IJsonObjectAdapter* p_deserialize = CreateJsonObjAdapter();
            p_deserialize->Parse( json_data.data() );

            ReportUtilities::DeserializeVector( *p_deserialize, true, str_oneD.c_str(),   read_oneD   );
            ReportUtilities::DeserializeVector( *p_deserialize, true, str_twoD.c_str(),   read_twoD   );
            ReportUtilities::DeserializeVector( *p_deserialize, true, str_threeD.c_str(), read_threeD );

            CHECK_ARRAY_EQUAL( oneD_a, read_oneD, uint32_t(oneD_a.size()) );

            CHECK_EQUAL( twoD_a.size(), read_twoD.size() );
            for( int i = 0; i < twoD_a.size(); ++i )
            {
                CHECK_ARRAY_EQUAL( twoD_a[i], read_twoD[i], uint32_t( twoD_a[i].size() ) );
            }

            CHECK_EQUAL( threeD.size(), read_threeD.size() );
            for( int i = 0; i < threeD.size(); ++i )
            {
                for( int j = 0; j < threeD[i].size(); ++j )
                {
                    CHECK_ARRAY_EQUAL( threeD[ i ][ j ], read_threeD[ i ][ j ], uint32_t( threeD[ i ][ j ].size() ) );
                }
            }

            delete p_deserialize;
            p_deserialize = nullptr;
        }
        catch( DetailedException& re )
        {
            PrintDebug( re.GetMsg() );
            PrintDebug( re.GetStackTrace() );
            CHECK( false );
        }
    }
}
