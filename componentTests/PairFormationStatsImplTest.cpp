/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "PairFormationStatsImpl.h"
#include "PairFormationParametersImpl.h"

using namespace std; 
using namespace Kernel; 

SUITE(PairFormationStatsImplTest)
{
    TEST(TestMethods)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PairFormationParametersTest/TransitoryParameters.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

        unique_ptr<IPairFormationStats> stats( PairFormationStatsImpl::CreateStats( from_data.get() ) );

        // -------------------------------------------------------
        // --- Verify that eligibility map is initialized to zero
        // -------------------------------------------------------
        const map<int, vector<int>>& eligible_map_low = stats->GetEligible( RiskGroup::LOW  );
        const map<int, vector<int>>& eligible_map_hgh = stats->GetEligible( RiskGroup::HIGH );

        CHECK_EQUAL( 2, eligible_map_low.size() );
        CHECK_EQUAL( 20, eligible_map_low.at( Gender::MALE   ).size() );
        CHECK_EQUAL( 20, eligible_map_low.at( Gender::FEMALE ).size() );
        CHECK_EQUAL( 2, eligible_map_hgh.size() );
        CHECK_EQUAL( 20, eligible_map_hgh.at( Gender::MALE   ).size() );
        CHECK_EQUAL( 20, eligible_map_hgh.at( Gender::FEMALE ).size() );

        for( int i = 0 ; i < 20 ; i++ )
        {
            CHECK_EQUAL( 0, eligible_map_low.at( Gender::MALE   )[i] );
            CHECK_EQUAL( 0, eligible_map_hgh.at( Gender::FEMALE )[i] );
        }

        // -----------------------------------------------------------
        // --- Add one eligible male and verify that it is in the map
        // -----------------------------------------------------------
        float age_m = 21.0f*365.0f ;
        stats->UpdateEligible( age_m, Gender::MALE, RiskGroup::LOW, 1 );

        for( int i = 0 ; i < 20 ; i++ )
        {
            if( i == 2 )
                CHECK_EQUAL( 1, eligible_map_low.at( Gender::MALE   )[i] );
            else
                CHECK_EQUAL( 0, eligible_map_low.at( Gender::MALE   )[i] );

            CHECK_EQUAL( 0, eligible_map_hgh.at( Gender::FEMALE )[i] );
        }

        // ---------------------------------------------------------------------------
        // --- Add two more eligible people and verify that they are added to the map
        // ---------------------------------------------------------------------------
        float age_f = 51.0f*365.0f ;
        stats->UpdateEligible( age_m, Gender::MALE,   RiskGroup::LOW, 1 );
        stats->UpdateEligible( age_f, Gender::FEMALE, RiskGroup::HIGH, 1 );

        for( int i = 0 ; i < 20 ; i++ )
        {
            if( i == 2 )
                CHECK_EQUAL( 2, eligible_map_low.at( Gender::MALE   )[i] );
            else
                CHECK_EQUAL( 0, eligible_map_low.at( Gender::MALE   )[i] );

            if( i == 14 )
                CHECK_EQUAL( 1, eligible_map_hgh.at( Gender::FEMALE )[i] );
            else
                CHECK_EQUAL( 0, eligible_map_hgh.at( Gender::FEMALE )[i] );
        }

        // ------------------------------------
        // --- Verify that we can reset the map
        // ------------------------------------
        stats->ResetEligible();
        for( int i = 0 ; i < 20 ; i++ )
        {
            CHECK_EQUAL( 0, eligible_map_low.at( Gender::MALE   )[i] );
            CHECK_EQUAL( 0, eligible_map_hgh.at( Gender::FEMALE )[i] );
        }

    }
}
