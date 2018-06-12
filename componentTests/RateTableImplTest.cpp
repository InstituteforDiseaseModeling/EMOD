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
#include "RateTableImpl.h"
#include "PairFormationParametersImpl.h"

using namespace std; 
using namespace Kernel; 

SUITE(RateTableImplTest)
{
    TEST(TestGetSetRates)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/PairFormationParametersTest/TransitoryParameters.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

        unique_ptr<IPairFormationRateTable> rate_table( RateTableImpl::CreateRateTable( from_data.get() ) );

        // ------------------------------------------------
        // --- Verify that the rate table starts off empty
        // ------------------------------------------------
        float age = 21.0f*365.0f ;
        CHECK_EQUAL( 0.0f, rate_table->GetRateForAgeAndSexAndRiskGroup( age, Gender::MALE,   RiskGroup::LOW  ) );
        CHECK_EQUAL( 0.0f, rate_table->GetRateForAgeAndSexAndRiskGroup( age, Gender::FEMALE, RiskGroup::HIGH ) );

        int bin_index_m = from_data->BinIndexForAgeAndSex( age, Gender::MALE   );
        int bin_index_f = from_data->BinIndexForAgeAndSex( age, Gender::FEMALE );

        CHECK_EQUAL( bin_index_m, bin_index_f );

        // -------------------------------------------------------------------------
        // --- Verify that we can set values in the table and retrieve those values
        // -------------------------------------------------------------------------
        rate_table->SetRateForBinAndSexAndRiskGroup( bin_index_m, Gender::MALE, RiskGroup::LOW, 7.7f );

        CHECK_EQUAL( 7.7f, rate_table->GetRateForAgeAndSexAndRiskGroup( age, Gender::MALE,   RiskGroup::LOW  ) );
        CHECK_EQUAL( 0.0f, rate_table->GetRateForAgeAndSexAndRiskGroup( age, Gender::FEMALE, RiskGroup::HIGH ) );

        rate_table->SetRateForBinAndSexAndRiskGroup( bin_index_m, Gender::FEMALE, RiskGroup::HIGH, 9.9f );

        CHECK_EQUAL( 7.7f, rate_table->GetRateForAgeAndSexAndRiskGroup( age, Gender::MALE,   RiskGroup::LOW  ) );
        CHECK_EQUAL( 9.9f, rate_table->GetRateForAgeAndSexAndRiskGroup( age, Gender::FEMALE, RiskGroup::HIGH ) );
    }
}
