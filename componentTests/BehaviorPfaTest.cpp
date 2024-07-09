
#include "stdafx.h"
#include <iostream>
#include <memory> // unique_ptr
#include "UnitTest++.h"
#include "ChiSquare.h"
#include "PfaFixture.h"

#include "BehaviorPfa.h"
#include "PairFormationParametersImpl.h"
#include "IdmString.h"
#include "NoCrtWarnings.h"

using namespace std; 
using namespace Kernel; 

SUITE(BehaviorPfaTest)
{

    TEST_FIXTURE(PfaFixture, TestAddRemoveIndividual)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/BehaviorPfaTest.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

        RandomFake fake_rng ;

        unique_ptr<IPairFormationAgent> pfa( BehaviorPfa::CreatePfa( p_config.get(), from_data.get(), 0.0f, &fake_rng,
                [this,&fake_rng]( IIndividualHumanSTI* male,
                                  IIndividualHumanSTI* female,
                                  bool isOutsidePFA,
                                  Sigmoid* pCondomUsage ) { AddRelationship( &fake_rng, male, female ); } ) );

        BehaviorPfa* bpfa = dynamic_cast<BehaviorPfa*>( pfa.get() );

        // --------------------------------------
        // --- Check that the population is empty
        // --------------------------------------
        CHECK_EQUAL( 0, bpfa->GetNumPopulationTotal()  );
        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            CHECK_EQUAL( 0, bpfa->GetNumMalesInBin(   i ) );
            CHECK_EQUAL( 0, bpfa->GetNumFemalesInBin( i ) );
        }

        // -----------------
        // --- Add one male
        // -----------------
        IndividualHumanContextFake* p_male = CreateHuman( Gender::MALE, 21.0f*365.0f ) ;
        pfa->AddIndividual( p_male );

        CHECK_EQUAL( 1, bpfa->GetNumPopulationTotal()  );
        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            if( i == 2 )
                CHECK_EQUAL( 1, bpfa->GetNumMalesInBin( i ) );
            else
                CHECK_EQUAL( 0, bpfa->GetNumMalesInBin( i ) );

            CHECK_EQUAL( 0, bpfa->GetNumFemalesInBin( i ) );
        }

        // ------------------
        // --- Add one female
        // ------------------
        IndividualHumanContextFake* p_female = CreateHuman( Gender::FEMALE, 51.0f*365.0f ) ;
        pfa->AddIndividual( p_female );

        CHECK_EQUAL( 2, bpfa->GetNumPopulationTotal()  );
        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            if( i == 2 )
                CHECK_EQUAL( 1, bpfa->GetNumMalesInBin( i ) );
            else
                CHECK_EQUAL( 0, bpfa->GetNumMalesInBin( i ) );

            if( i == 14 )
                CHECK_EQUAL( 1, bpfa->GetNumFemalesInBin( i ) );
            else
                CHECK_EQUAL( 0, bpfa->GetNumFemalesInBin( i ) );
        }

        // ------------------
        // --- Add second female
        // ------------------
        IndividualHumanContextFake* p_female2 = CreateHuman( Gender::FEMALE, 81.0f*365.0f ) ;
        pfa->AddIndividual( p_female2 );

        CHECK_EQUAL( 3, bpfa->GetNumPopulationTotal()  );
        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            if( i == 2 )
                CHECK_EQUAL( 1, bpfa->GetNumMalesInBin( i ) );
            else
                CHECK_EQUAL( 0, bpfa->GetNumMalesInBin( i ) );

            if( i == 14 )
                CHECK_EQUAL( 1, bpfa->GetNumFemalesInBin( i ) );
            else if( i == 19 )
                CHECK_EQUAL( 1, bpfa->GetNumFemalesInBin( i ) );
            else
                CHECK_EQUAL( 0, bpfa->GetNumFemalesInBin( i ) );
        }

        // ----------------
        // --- Remove Male
        // ----------------
        pfa->RemoveIndividual( p_male );
        CHECK_EQUAL( 2, bpfa->GetNumPopulationTotal()  );
        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
             CHECK_EQUAL( 0, bpfa->GetNumMalesInBin( i ) );

            if( i == 14 )
                CHECK_EQUAL( 1, bpfa->GetNumFemalesInBin( i ) );
            else if( i == 19 )
                CHECK_EQUAL( 1, bpfa->GetNumFemalesInBin( i ) );
            else
                CHECK_EQUAL( 0, bpfa->GetNumFemalesInBin( i ) );
        }

        // ----------------
        // --- Remove Female #1
        // ----------------
        pfa->RemoveIndividual( p_female );
        CHECK_EQUAL( 1, bpfa->GetNumPopulationTotal()  );
        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            CHECK_EQUAL( 0, bpfa->GetNumMalesInBin( i ) );

            if( i == 19 )
                CHECK_EQUAL( 1, bpfa->GetNumFemalesInBin( i ) );
            else
                CHECK_EQUAL( 0, bpfa->GetNumFemalesInBin( i ) );
        }

        // ----------------
        // --- Remove Female #2
        // ----------------
        pfa->RemoveIndividual( p_female2 );
        CHECK_EQUAL( 0, bpfa->GetNumPopulationTotal()  );
        for( int i = 0 ; i < from_data->GetMaleAgeBinCount() ; i++ )
        {
            CHECK_EQUAL( 0, bpfa->GetNumMalesInBin(   i ) );
            CHECK_EQUAL( 0, bpfa->GetNumFemalesInBin( i ) );
        }

        CHECK_EQUAL( 0, GetNumRelationships() );
    }

    TEST_FIXTURE(PfaFixture, TestUpdateBasic)
    {
        IdmDateTime date_time_2000( 2000.0f * 365.0f ) ;

        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/BehaviorPfaTest.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

        RandomFake fake_rng ;

        unique_ptr<IPairFormationAgent> pfa( BehaviorPfa::CreatePfa( p_config.get(), from_data.get(), 0.0f, &fake_rng,
                [this,&fake_rng]( IIndividualHumanSTI* male,
                                  IIndividualHumanSTI* female,
                                  bool isOutsidePFA,
                                  Sigmoid* pCondomUsage ) { AddRelationship( &fake_rng, male, female ); } ) );

        BehaviorPfa* bpfa = dynamic_cast<BehaviorPfa*>( pfa.get() );

        CHECK_EQUAL( 0, GetNumRelationships() );

        pfa->Update( date_time_2000, 1.0f );

        CHECK_EQUAL( 0, GetNumRelationships() );

        // -------------------------------------------------------------------
        // --- Create two individuals should be able to create a relationship
        // -------------------------------------------------------------------
        pfa->AddIndividual( CreateHuman( Gender::MALE,   18.0f*365.0f ) );
        pfa->AddIndividual( CreateHuman( Gender::FEMALE, 18.0f*365.0f ) );

        pfa->Update( date_time_2000, 1.0f );

        CHECK_EQUAL( 1, GetNumRelationships() );
        CHECK_EQUAL( 1, GetHuman(0)->GetRelationships().size() ); // 18y-male
        CHECK_EQUAL( 1, GetHuman(1)->GetRelationships().size() ); // 18y-female

        // ---------------------------------------------------------------------
        // --- Create two more individuals who's age difference is great enough
        // --- that they won't form a relationship.
        // ---------------------------------------------------------------------
        pfa->AddIndividual( CreateHuman( Gender::MALE,   28.0f*365.0f ) );
        pfa->AddIndividual( CreateHuman( Gender::FEMALE, 68.0f*365.0f ) );

        pfa->Update( date_time_2000, 1.0f );

        CHECK_EQUAL( 1, GetNumRelationships() );
        CHECK_EQUAL( 1, GetHuman(0)->GetRelationships().size() ); // 18y-male
        CHECK_EQUAL( 1, GetHuman(1)->GetRelationships().size() ); // 18y-female
        CHECK_EQUAL( 0, GetHuman(2)->GetRelationships().size() ); // 28y-male
        CHECK_EQUAL( 0, GetHuman(3)->GetRelationships().size() ); // 68y-female

        // ---------------------------------------------------------------------
        // --- Create another individual closer in age to the older women and
        // --- see the relationship created.
        // ---------------------------------------------------------------------
        pfa->AddIndividual( CreateHuman( Gender::MALE,   71.0f*365.0f ) );

        pfa->Update( date_time_2000, 1.0f );

        CHECK_EQUAL( 2, GetNumRelationships() );
        CHECK_EQUAL( 1, GetHuman(0)->GetRelationships().size() ); // 18y-male
        CHECK_EQUAL( 1, GetHuman(1)->GetRelationships().size() ); // 18y-female
        CHECK_EQUAL( 0, GetHuman(2)->GetRelationships().size() ); // 28y-male
        CHECK_EQUAL( 1, GetHuman(3)->GetRelationships().size() ); // 68y-female
        CHECK_EQUAL( 1, GetHuman(4)->GetRelationships().size() ); // 71y-male
    }

    TEST_FIXTURE(PfaFixture, TestDistributions)
    {
        unique_ptr<Configuration> p_config( Environment::LoadConfigurationFile( "testdata/BehaviorPfaTest.json" ) );

        unique_ptr<IPairFormationParameters> from_data( PairFormationParametersImpl::CreateParameters( RelationshipType::TRANSITORY, p_config.get() ) );

        PSEUDO_DES rng ;

        IdmDateTime date_time_2000( 2000.0f * 365.0f ) ;

        // --------------------------------------------------------------------------------------
        // --- If NUM_MALES = 1000, the distibution for 57.5 year olds doesn't pass the CS test
        // --- if our significance is 0.05 (i.e. 0.95).  If we lower our significance to
        // --- 0.025 (i.e. 0.975), then the test passes.  
        // --- If we switch NUM_MALES to 800 and leave our sigificance at 0.05, the CS test passes.
        // --- I'm pretty sure it is a rngdom number issue, because if we start the testing with
        // --- the 57.5 year old age bin, everything is ok.
        // --- I ran each male_bin_index 100 times with NUM_MALES=1000 and saw that up to
        // --- 10% of the time a distribution will not match at significance of 0.05 and 6% at 0.025.
        // --------------------------------------------------------------------------------------
        const int NUM_MALES = 1000 ;
        const int NUM_FEMALES_PER_BIN = NUM_MALES  ;
        float percent_infected = 0.5 ;

        std::vector<int> num_passed_list ;
        for( int male_bin_index = 0 ; male_bin_index < from_data->GetMaleAgeBinCount() ; male_bin_index++ )
        {
            unique_ptr<IPairFormationAgent> pfa( BehaviorPfa::CreatePfa( p_config.get(), from_data.get(), 0.0f, &rng,
                    [this, &rng ]( IIndividualHumanSTI* male,
                                   IIndividualHumanSTI* female,
                                   bool isOutsidePFA,
                                   Sigmoid* pCondomUsage ) { AddRelationship( &rng, male, female ); } ) );

            BehaviorPfa* bpfa = dynamic_cast<BehaviorPfa*>( pfa.get() );

            // ---------------------------------------------------------------------------------------------
            // --- For each call to BehaviorPfa::Update(), we put NUM_MALES into a single age bin.
            // --- Then, we want the same number of females in each bin.
            // -------------------------------------------------------------------------------
            // --- The -0.1 is to fix the test after fixing the initial value of the age bins.
            // --- The first bin used to really be [0, initial_value + increment) but is now
            // --- [0, initial_value).  The test logic assumed giving the initial value put it
            // --- in that bin.
            // ---------------------------------------------------------------------------------------------
            float male_age_years = from_data->GetInitialMaleAge() + (from_data->GetMaleAgeIncrement() * float(male_bin_index)) - 0.1;
            float male_age_days = male_age_years * DAYSPERYEAR ;
            for( int m = 0 ; m < NUM_MALES ; m++ )
            {
                pfa->AddIndividual( CreateHuman( Gender::MALE, male_age_days, percent_infected ) );
            }

            for( int female_bin_index = 0 ; female_bin_index < from_data->GetFemaleAgeBinCount() ; female_bin_index++ )
            {
                float female_age_years = from_data->GetInitialFemaleAge() + (from_data->GetFemaleAgeIncrement() * float(female_bin_index)) - 0.1 ; // see note above
                float female_age_days = female_age_years * DAYSPERYEAR ;
                for( int f = 0 ; f < NUM_FEMALES_PER_BIN ; f++ )
                {
                    pfa->AddIndividual( CreateHuman( Gender::FEMALE, female_age_days, percent_infected ) );
                }
            }

            // -----------------------------------------------------
            // --- Create as many relationships as we can given the 
            // --- distribution of the males and females.
            // -----------------------------------------------------
            pfa->Update( date_time_2000, 1.0f );

            // --------------------------------------------------------
            // --- Now verify that the distribution of females in a 
            // --- relationship with a male of this age is as expected.
            // --------------------------------------------------------
            vector<float> expected ;
            vector<float> actual ;
            std::wstringstream wss_exp ;
            std::wstringstream wss_act ;
            for( int female_bin_index = 0 ; female_bin_index < from_data->GetFemaleAgeBinCount() ; female_bin_index++ )
            {
                int act_in_relationship = NUM_FEMALES_PER_BIN - bpfa->GetNumFemalesInBin( female_bin_index ) ;

                float exp_in_relationship = float(NUM_MALES)*from_data->JointProbabilityTable()[ male_bin_index ][ female_bin_index ] ;

                actual.push_back( act_in_relationship );
                expected.push_back( exp_in_relationship );

                //wss_act << act_in_relationship << "\n" ;
                //wss_exp << exp_in_relationship << "\n" ;
            }

            //wss_act << "\n" ;
            //wss_exp << "\n" ;
            //OutputDebugStringW( wss_exp.str().c_str() );
            //OutputDebugStringW( wss_act.str().c_str() );

            int df = -1 ;
            float chi_square_stat = 0.0;
            ChiSquare::CalculateChiSquareStatistic( 5.0f, expected, actual, &chi_square_stat, &df );
            float chi_square_critical_value = ChiSquare::GetChiSquareCriticalValue( df );

            bool passed = chi_square_critical_value > chi_square_stat ;
            printf("age=%f  df=%d  stat=%f  cv=%f  passed=%d\n",male_age_years,df,chi_square_stat,chi_square_critical_value,passed);

            CHECK( chi_square_critical_value > chi_square_stat );

            // ----------------------------------------------------------
            // --- Verify that each persion is in only one relationship.
            // ----------------------------------------------------------
            for( int h = 0 ; h < GetNumHumans() ; h++ )
            {
                CHECK( 1 >= GetHuman(h)->GetRelationships().size() );
            }
            ClearData();
        }
    }
}
