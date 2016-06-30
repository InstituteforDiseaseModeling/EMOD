/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "FlowControllerImpl.h"
#include "PairFormationParametersImpl.h"
#include "PairFormationStatsImpl.h"
#include "RateTableImpl.h"
#include "BehaviorPFA.h"
#include "../utils/Common.h"
#include "SimulationConfig.h"

using namespace std; 
using namespace Kernel; 

SUITE(FlowControllerImplTest)
{
    struct FlowControllerFixture
    {
        FlowControllerFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
        }

        ~FlowControllerFixture()
        {
            Environment::Finalize();
        }
    };

    TEST_FIXTURE(FlowControllerFixture, TestGetSetRates)
    {
        SimulationConfig* p_sim_config = new SimulationConfig();
        p_sim_config->sim_type = SimType::HIV_SIM ;
        Environment::setSimulationConfig( p_sim_config );

        PSEUDO_DES ran ;

        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/FlowControllerImplTest.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

        unique_ptr<IPairFormationRateTable> rate_table( RateTableImpl::CreateRateTable( from_data.get() ) );
        unique_ptr<IPairFormationStats> stats( PairFormationStatsImpl::CreateStats( from_data.get() ) );
        unique_ptr<IPairFormationAgent> pfa( BehaviorPfa::CreatePfa( p_config.get(), from_data.get(), 0.2f, &ran,
                [this](IIndividualHumanSTI* male, IIndividualHumanSTI* female) { /*AddRelationship( male, female );*/ } ) );

        unique_ptr<IPairFormationFlowController> controller( FlowControllerImpl::CreateController( pfa.get(), 
                                                                                                   stats.get(), 
                                                                                                   rate_table.get(), 
                                                                                                   from_data.get() ) );

        // --------------------------------------------------------------------
        // --- Verify that the rates and eligibility are zero at the beginning
        // --------------------------------------------------------------------
        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            float age_years = from_data->GetInitialMaleAge() + (i * from_data->GetMaleAgeIncrement());
            float age_days  = age_years * DAYSPERYEAR ;

            CHECK_EQUAL( 0.0f, rate_table->GetRateForAgeAndSexAndRiskGroup( age_days, Gender::MALE,   RiskGroup::LOW  ) );
            CHECK_EQUAL( 0.0f, rate_table->GetRateForAgeAndSexAndRiskGroup( age_days, Gender::FEMALE, RiskGroup::HIGH ) );

            CHECK_EQUAL( 0, stats->GetEligible( RiskGroup::LOW  ).at( Gender::MALE   )[i] );
            CHECK_EQUAL( 0, stats->GetEligible( RiskGroup::HIGH ).at( Gender::FEMALE )[i] );
        }

        // -----------------------------------------------------
        // --- Verify that if the elibility and rates are zero, 
        // --- then calling update doesn't change anything.
        // -----------------------------------------------------
        IdmDateTime current_time;
        controller->UpdateEntryRates( current_time, 1.0 );

        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            float age_years = from_data->GetInitialMaleAge() + (i * from_data->GetMaleAgeIncrement());
            float age_days  = age_years * DAYSPERYEAR ;

            CHECK_EQUAL( 0.0f, rate_table->GetRateForAgeAndSexAndRiskGroup( age_days, Gender::MALE,   RiskGroup::LOW  ) );
            CHECK_EQUAL( 0.0f, rate_table->GetRateForAgeAndSexAndRiskGroup( age_days, Gender::FEMALE, RiskGroup::HIGH ) );

            CHECK_EQUAL( 0, stats->GetEligible( RiskGroup::LOW  ).at( Gender::MALE   )[i] );
            CHECK_EQUAL( 0, stats->GetEligible( RiskGroup::HIGH ).at( Gender::FEMALE )[i] );
        }

        // -----------------------------------------------------------------
        // --- Indicate that there are an increasing number of people
        // --- in each bin.  Call UpdateEntryRates() and see that we get 
        // --- the expected values.
        // -----------------------------------------------------------------

        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            // -------------------------------------------------------------------------------
            // --- The -0.1 is to fix the test after fixing the initial value of the age bins.
            // --- The first bin used to really be [0, initial_value + increment) but is now
            // --- [0, initial_value).  The test logic assumed giving the initial value put it
            // --- in that bin.
            // -------------------------------------------------------------------------------
            float age_years = from_data->GetInitialMaleAge() + (i * from_data->GetMaleAgeIncrement()) - 0.1;
            float age_days  = age_years * DAYSPERYEAR ;

            stats->UpdateEligible( age_days, Gender::MALE,   RiskGroup::LOW,  10*(i+1) );
            stats->UpdateEligible( age_days, Gender::FEMALE, RiskGroup::HIGH, 20*(i+1) );
        }

        controller->UpdateEntryRates( current_time, 1.0 );

        float exp_rates_m[] = { 0.048263057f, 0.025510424f, 0.019590163f, 0.016030330f, 0.011445053f, 
                                0.007249568f, 0.004470678f, 0.002753004f, 0.001933519f, 0.001506529f,
                                0.001059091f, 0.000685632f, 0.000483936f, 0.000368111f, 0.000255812f,
                                0.000166353f, 0.000122230f, 0.000106398f, 0.000100227f, 9.82145E-05f };

        float exp_rates_f[] = { 0.043975705f, 0.020889865f, 0.012323607f, 0.007428332f, 0.004282167f,
                                0.002484300f, 0.001550387f, 0.000964758f, 0.000553777f, 0.000314833f,
                                0.000203960f, 0.000153069f, 0.000125059f, 0.000102745f, 8.02943E-05f,
                                5.96776E-05f, 4.31564E-05f, 3.32031E-05f, 2.89282E-05f, 2.69677E-05f };

        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            float age_years = from_data->GetInitialMaleAge() + (i * from_data->GetMaleAgeIncrement()) - 0.1; // note above
            float age_days  = age_years * DAYSPERYEAR ;

            CHECK_CLOSE( exp_rates_m[i], rate_table->GetRateForAgeAndSexAndRiskGroup( age_days, Gender::MALE,   RiskGroup::LOW  ), 0.000001 );
            CHECK_CLOSE( exp_rates_f[i], rate_table->GetRateForAgeAndSexAndRiskGroup( age_days, Gender::FEMALE, RiskGroup::HIGH ), 0.000001 );
        }
    }
}
