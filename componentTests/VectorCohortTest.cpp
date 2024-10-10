
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "componentTests.h"
#include "VectorCohort.h"
#include "VectorContexts.h"
#include "RANDOM.h"
#include "Instrumentation.h"
#include "Environment.h"
#include "Log.h"

using namespace Kernel;

SUITE( VectorCohortTest )
{
    struct VectorCohortFixture
    {
        VectorCohortFixture()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
        }

        ~VectorCohortFixture()
        {
            Environment::Finalize();
        }
    };

    TEST_FIXTURE( VectorCohortFixture, TestCreate )
    {
        VectorGenome genome_empty;
        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );
        genome_self.SetLocus( 2, 0, 1 );
        genome_self.SetLocus( 3, 1, 1 );
        genome_self.SetWolbachia( VectorWolbachia::VECTOR_WOLBACHIA_AB );

        VectorGenome genome_mate;
        genome_mate.SetLocus( 0, 0, 1 ); //male
        genome_mate.SetLocus( 1, 0, 1 );
        genome_mate.SetLocus( 2, 1, 1 );
        genome_mate.SetLocus( 3, 0, 0 );

        std::unique_ptr<IVectorCohort> pvc( VectorCohort::CreateCohort( 1,
                                                                        VectorStateEnum::STATE_IMMATURE,
                                                                        1.0f,
                                                                        0.1f,
                                                                        7.0,
                                                                        99,
                                                                        genome_self,
                                                                        0 ) );

        CHECK_EQUAL( VectorStateEnum::STATE_IMMATURE, pvc->GetState() );
        CHECK_EQUAL( 1.0f, pvc->GetAge() );
        CHECK_EQUAL( 0.1f, pvc->GetProgress() );
        CHECK_EQUAL( 99, pvc->GetPopulation() );
        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        CHECK_EQUAL( genome_empty.GetBits(), pvc->GetMateGenome().GetBits() );
        CHECK_EQUAL( 0, pvc->RemoveNumDoneGestating() );
        CHECK_EQUAL( 0, pvc->GetGestatingQueue().size() );
        CHECK_EQUAL( VectorWolbachia::VECTOR_WOLBACHIA_AB, pvc->GetWolbachia() );
        CHECK_EQUAL( 0, pvc->GetIMigrate()->GetMigrationDestination().data );
        CHECK_EQUAL( MigrationType::NO_MIGRATION, pvc->GetIMigrate()->GetMigrationType() );
        CHECK_EQUAL( 7.0, pvc->GetDurationOfMicrosporidia() );

        pvc->SetState( VectorStateEnum::STATE_ADULT );
        CHECK_EQUAL( VectorStateEnum::STATE_ADULT, pvc->GetState() );

        pvc->SetAge( 3.0 );
        CHECK_EQUAL( 3.0, pvc->GetAge() );

        pvc->IncreaseAge( 1.0 );
        CHECK_EQUAL( 4.0, pvc->GetAge() );

        pvc->SetPopulation( 77 );
        CHECK_EQUAL( 77, pvc->GetPopulation() );

        pvc->SetMateGenome( genome_mate );
        CHECK_EQUAL( genome_mate.GetBits(), pvc->GetMateGenome().GetBits() );

        CHECK_EQUAL( 0.1f, pvc->GetProgress() );
        pvc->ClearProgress( );
        CHECK_EQUAL( 0.0f, pvc->GetProgress() );

        suids::suid id;
        id.data = 3;
        pvc->GetIMigrate()->SetMigrating( id, MigrationType::LOCAL_MIGRATION, 0.0, 0.0, false );
        CHECK_EQUAL( 3, pvc->GetIMigrate()->GetMigrationDestination().data );
        CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, pvc->GetIMigrate()->GetMigrationType() );
    }

    TEST_FIXTURE( VectorCohortFixture, TestGestation )
    {
        PSEUDO_DES rng( 2 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );
        genome_self.SetLocus( 2, 0, 1 );
        genome_self.SetLocus( 3, 1, 1 );

        VectorGenome genome_mate;
        genome_mate.SetLocus( 0, 0, 1 ); //male
        genome_mate.SetLocus( 1, 0, 1 );
        genome_mate.SetLocus( 2, 1, 1 );
        genome_mate.SetLocus( 3, 0, 0 );

        std::unique_ptr<IVectorCohort> pvc( VectorCohort::CreateCohort( 1,
                                                                        VectorStateEnum::STATE_IMMATURE,
                                                                        1.0f,
                                                                        0.0f,
                                                                        0.0f,
                                                                        100,
                                                                        genome_self,
                                                                        0 ) );

        pvc->SetMateGenome( genome_mate );
        CHECK_EQUAL( genome_mate.GetBits(), pvc->GetMateGenome().GetBits() );
        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        CHECK_EQUAL( 100, pvc->GetPopulation() );
        CHECK_EQUAL( 0, pvc->RemoveNumDoneGestating() );
        CHECK_EQUAL( 0, pvc->GetGestatingQueue().size() );

        pvc->AddNewGestating( 3, 20 );
        pvc->AddNewGestating( 4, 10 );

        std::vector<uint32_t> expected = { 0, 0, 20, 10 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        pvc->AdjustGestatingForDeath( &rng, 0, false );
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        CHECK_EQUAL( 0, pvc->RemoveNumDoneGestating() );
        expected = { 0, 20, 10, 0 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        CHECK_EQUAL( 0, pvc->RemoveNumDoneGestating() );
        expected = { 20, 10, 0, 0 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        CHECK_EQUAL( 20, pvc->RemoveNumDoneGestating() );
        expected = { 10, 0, 0, 0 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        CHECK_EQUAL( 10, pvc->RemoveNumDoneGestating() );
        expected = { 0, 0, 0, 0 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        pvc->SetPopulation( 10 );
        uint32_t num_day2 = 3;
        uint32_t num_day3 = 6;
        pvc->AddNewGestating( 2, num_day2 );
        pvc->AddNewGestating( 3, num_day3 );
        expected = { 0, num_day2, num_day3, 0 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );
        CHECK_EQUAL( 9, pvc->GetNumGestating() );

        uint32_t num_samples = 100000;
        for( uint32_t num_died = 1; num_died <= pvc->GetPopulation(); num_died+=1 )
        {
            float num_died_percentage = float( num_died ) / float( pvc->GetPopulation() );
            uint32_t total_died = 0;
            uint32_t total_died_day2 = 0;
            uint32_t total_died_day3 = 0;
            uint32_t total_died_not = 0;
            for( uint32_t i = 0; i < num_samples; ++i )
            {
                uint32_t starting_pop = pvc->GetPopulation();
                uint32_t num_died_actual = pvc->AdjustGestatingForDeath( &rng, num_died_percentage, false );
                CHECK_EQUAL( 4, pvc->GetGestatingQueue().size() );
                CHECK_EQUAL( 0, pvc->GetGestatingQueue()[ 0 ] );
                CHECK_EQUAL( 0, pvc->GetGestatingQueue()[ 3 ] );

                uint32_t died = starting_pop - pvc->GetPopulation();
                uint32_t died_day2 = num_day2 - pvc->GetGestatingQueue()[ 1 ];
                uint32_t died_day3 = num_day3 - pvc->GetGestatingQueue()[ 2 ];
                uint32_t died_not = died - died_day2 - died_day3;
                //printf("%d,%d,%d,%d,%d,%d\n",num_died, died, total_died, died_day2,died_day3,died_not);

                total_died      += died;
                total_died_day2 += died_day2;
                total_died_day3 += died_day3;
                total_died_not  += died_not;

                pvc->SetPopulation( 10 );
                pvc->AddNewGestating( 2, died_day2 );
                pvc->AddNewGestating( 3, died_day3 );
                CHECK_EQUAL( 9, pvc->GetNumGestating() );

                CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
                CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );
            }
            float act_avg_died          = float( total_died ) / float( num_samples );
            float act_percent_died_day2 = float( total_died_day2 ) / float( num_samples ) / float( num_died );
            float act_percent_died_day3 = float( total_died_day3 ) / float( num_samples ) / float( num_died );
            float act_percent_died_not  = float( total_died_not  ) / float( num_samples ) / float( num_died );

            float exp_percent_died_day2 = float( num_day2 ) / float( pvc->GetPopulation() );
            float exp_percent_died_day3 = float( num_day3 ) / float( pvc->GetPopulation() );
            float exp_percent_died_not  = float( pvc->GetPopulation() - num_day2 - num_day3 ) / float( pvc->GetPopulation() );

            //printf("%d,%f,%f,%f,%f,%f,%f,%f\n",num_died,act_avg_died,
            //        exp_percent_died_day2, exp_percent_died_day3, exp_percent_died_not,
            //        act_percent_died_day2, act_percent_died_day3, act_percent_died_not );

            CHECK_CLOSE( float( num_died ),     act_avg_died,          0.01 );
            CHECK_CLOSE( exp_percent_died_day2, act_percent_died_day2, 0.01 );
            CHECK_CLOSE( exp_percent_died_day3, act_percent_died_day3, 0.01 );
            CHECK_CLOSE( exp_percent_died_not,  act_percent_died_not,  0.01 );
        }
    }

    TEST_FIXTURE( VectorCohortFixture, TestSplitPercent )
    {
        PSEUDO_DES rng( 2 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );
        genome_self.SetLocus( 2, 0, 1 );
        genome_self.SetLocus( 3, 1, 1 );

        VectorGenome genome_mate;
        genome_mate.SetLocus( 0, 0, 1 ); //male
        genome_mate.SetLocus( 1, 0, 1 );
        genome_mate.SetLocus( 2, 1, 1 );
        genome_mate.SetLocus( 3, 0, 0 );

        std::unique_ptr<IVectorCohort> pvc( VectorCohort::CreateCohort( 1,
                                                                        VectorStateEnum::STATE_IMMATURE,
                                                                        1.0f,
                                                                        0.0f,
                                                                        0.0f,
                                                                        100,
                                                                        genome_self,
                                                                        0 ) );

        pvc->SetMateGenome( genome_mate );
        CHECK_EQUAL( genome_mate.GetBits(), pvc->GetMateGenome().GetBits() );
        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        CHECK_EQUAL( 100, pvc->GetPopulation() );
        CHECK_EQUAL( 0, pvc->RemoveNumDoneGestating() );
        CHECK_EQUAL( 0, pvc->GetGestatingQueue().size() );

        pvc->AddNewGestating( 3, 30 );
        pvc->AddNewGestating( 4, 60 );
        std::vector<uint32_t> expected = { 0, 0, 30, 60 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        std::unique_ptr<IVectorCohort> split_pvc( pvc->SplitPercent( &rng, 2, 0.5 ) );

        CHECK_EQUAL( genome_mate.GetBits(), split_pvc->GetMateGenome().GetBits() );
        CHECK_EQUAL( genome_self.GetBits(), split_pvc->GetGenome().GetBits() );
        CHECK_EQUAL( 57, split_pvc->GetPopulation() );
        expected = { 0, 0, 17, 34 };
        CHECK_EQUAL( expected.size(), split_pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, split_pvc->GetGestatingQueue(), expected.size() );

        CHECK_EQUAL( genome_mate.GetBits(), pvc->GetMateGenome().GetBits() );
        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        CHECK_EQUAL( 43, pvc->GetPopulation() );
        expected = { 0, 0, 13, 26 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        pvc->Merge( split_pvc.get() );

        CHECK_EQUAL( 100, pvc->GetPopulation() );
        expected = { 0, 0, 30, 60 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        // ---------------------------------------------------------------------------
        // --- Test that SplitPercent() using percentage works when no vectors are gestating
        // ---------------------------------------------------------------------------
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();

        CHECK_EQUAL(   0, pvc->GetNumGestating() );
        CHECK_EQUAL( 100, pvc->GetPopulation() );

        std::unique_ptr<IVectorCohort> split_pvc3( pvc->SplitPercent( &rng, 3, 0.5 ) );

        CHECK_EQUAL( 47, split_pvc3->GetPopulation() );
        CHECK_EQUAL( 53,        pvc->GetPopulation() );

        // ---------------------------------------------------------------------------
        // --- Test that SplitPercent() using percentage works when percentage is very small
        // --- It will return nullptr because binomial_approx() will return 0.
        // ---------------------------------------------------------------------------
        std::unique_ptr<IVectorCohort> pvc9( VectorCohort::CreateCohort( 9,
                                                                         VectorStateEnum::STATE_IMMATURE,
                                                                         1.0f,
                                                                         0.0f,
                                                                         0.0f,
                                                                         1000000,
                                                                         genome_self,
                                                                         0 ) );

        CHECK_EQUAL(       0, pvc9->GetNumGestating() );
        CHECK_EQUAL( 1000000, pvc9->GetPopulation() );

        IVectorCohort* p_expect_to_be_null = pvc9->SplitPercent( &rng, 10, 0.000001f );

        CHECK_EQUAL( (IVectorCohort*)nullptr, p_expect_to_be_null );
    }

    TEST_FIXTURE( VectorCohortFixture, TestSplitNumber )
    {
        PSEUDO_DES rng( 2 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );
        genome_self.SetLocus( 2, 0, 1 );
        genome_self.SetLocus( 3, 1, 1 );

        VectorGenome genome_mate;
        genome_mate.SetLocus( 0, 0, 1 ); //male
        genome_mate.SetLocus( 1, 0, 1 );
        genome_mate.SetLocus( 2, 1, 1 );
        genome_mate.SetLocus( 3, 0, 0 );

        std::unique_ptr<IVectorCohort> pvc( VectorCohort::CreateCohort( 1,
                                                                        VectorStateEnum::STATE_IMMATURE,
                                                                        1.0f,
                                                                        0.0f,
                                                                        0.0f,
                                                                        100,
                                                                        genome_self,
                                                                        0 ) );

        pvc->SetMateGenome( genome_mate );
        CHECK_EQUAL( genome_mate.GetBits(), pvc->GetMateGenome().GetBits() );
        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        CHECK_EQUAL( 100, pvc->GetPopulation() );
        CHECK_EQUAL( 0, pvc->RemoveNumDoneGestating() );
        CHECK_EQUAL( 0, pvc->GetGestatingQueue().size() );

        // ----------------------------------
        // --- Test SplitNumber() using num leaving
        // ----------------------------------

        std::unique_ptr<IVectorCohort> split_pvc2( pvc->SplitNumber( &rng, 2, 13 ) );

        CHECK_EQUAL( 13, split_pvc2->GetPopulation() );
        CHECK_EQUAL( 87,        pvc->GetPopulation() );

        pvc->Merge( split_pvc2.get() );

        CHECK_EQUAL(   0, pvc->GetNumGestating() );
        CHECK_EQUAL( 100, pvc->GetPopulation() );

        // --------------------------------------------------------------------
        // --- Test SplitNumber() using num leaving works when vectors are gestating
        // --------------------------------------------------------------------
        pvc->AddNewGestating( 3, 5 );
        pvc->AddNewGestating( 4, 59 );
        std::vector<uint32_t> expected = { 0, 0, 5, 59 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        std::unique_ptr<IVectorCohort> split_pvc5( pvc->SplitNumber( &rng, 5, 43 ) );

        CHECK_EQUAL( 43, split_pvc5->GetPopulation() );
        CHECK_EQUAL( 57,        pvc->GetPopulation() );

        expected = { 0, 0,  4, 32 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );
        expected = { 0, 0,  1, 27 };
        CHECK_EQUAL( expected.size(), split_pvc5->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, split_pvc5->GetGestatingQueue(), expected.size() );

        pvc->Merge( split_pvc5.get() );
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();

        CHECK_EQUAL(   0, pvc->GetNumGestating() );
        CHECK_EQUAL( 100, pvc->GetPopulation() );

        pvc->AddNewGestating( 3, 51 );
        pvc->AddNewGestating( 4, 17 );
        expected = { 0, 0, 51, 17 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        std::unique_ptr<IVectorCohort> split_pvc6( pvc->SplitNumber( &rng, 5, 37 ) );

        CHECK_EQUAL( 37, split_pvc6->GetPopulation() );
        CHECK_EQUAL( 63,        pvc->GetPopulation() );

        expected = { 0, 0,  27, 15 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );
        expected = { 0, 0,  24, 2 };
        CHECK_EQUAL( expected.size(), split_pvc6->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, split_pvc6->GetGestatingQueue(), expected.size() );

        // -------------------------------------------------------------------------------
        // --- Test SplitNumber() when we are splitting all of them
        // -------------------------------------------------------------------------------
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();

        pvc->SetPopulation( 2 );
        pvc->AddNewGestating( 3, 1 );

        expected = { 0, 0, 1, 0 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );

        std::unique_ptr<IVectorCohort> split_pvc7( pvc->SplitNumber( &rng, 7, 2 ) );

        CHECK_EQUAL( 2, split_pvc7->GetPopulation() );
        CHECK_EQUAL( 0,        pvc->GetPopulation() );

        CHECK_EQUAL( 1, split_pvc7->GetNumGestating() );
        CHECK_EQUAL( 0,        pvc->GetNumGestating() );

        expected = { 0, 0, 0, 0 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );
        expected = { 0, 0,  1, 0 };
        CHECK_EQUAL( expected.size(), split_pvc7->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, split_pvc7->GetGestatingQueue(), expected.size() );

        // -------------------------------------------------------------------------------
        // --- Test SplitNumber() with fixed numLeaving over a set of values and random numbers
        // -------------------------------------------------------------------------------
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();
        pvc->RemoveNumDoneGestating();

        uint32_t orig_population = 10;
        pvc->SetPopulation( orig_population );
        uint32_t num_day2 = 5;
        uint32_t num_day3 = 2;
        uint32_t exp_num_gestating = num_day2 + num_day3;
        pvc->AddNewGestating( 2, num_day2 );
        pvc->AddNewGestating( 3, num_day3 );
        expected = { 0, num_day2, num_day3, 0 };
        CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
        CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );
        CHECK_EQUAL( exp_num_gestating, pvc->GetNumGestating() );

        uint32_t num_samples = 100000;
        for( uint32_t num_leaving = 1; num_leaving <= pvc->GetPopulation(); num_leaving += 1 )
        {
            uint32_t total_leaving = 0;
            uint32_t total_leaving_day2 = 0;
            uint32_t total_leaving_day3 = 0;
            uint32_t total_leaving_not = 0;

            for( uint32_t i = 0; i < num_samples; ++i )
            {
                std::unique_ptr<IVectorCohort> leaving_pvc( pvc->SplitNumber( &rng, 5, num_leaving ) );

                CHECK(         pvc->GetPopulation() >=         pvc->GetNumGestating() );
                CHECK( leaving_pvc->GetPopulation() >= leaving_pvc->GetNumGestating() );

                CHECK_EQUAL( 4, pvc->GetGestatingQueue().size() );
                CHECK_EQUAL( 0, pvc->GetGestatingQueue()[ 0 ] );
                CHECK_EQUAL( 0, pvc->GetGestatingQueue()[ 3 ] );
                CHECK_EQUAL( 4, leaving_pvc->GetGestatingQueue().size() );
                CHECK_EQUAL( 0, leaving_pvc->GetGestatingQueue()[ 0 ] );
                CHECK_EQUAL( 0, leaving_pvc->GetGestatingQueue()[ 3 ] );

                CHECK_EQUAL( expected[ 1 ], (pvc->GetGestatingQueue()[ 1 ] + leaving_pvc->GetGestatingQueue()[ 1 ]) );
                CHECK_EQUAL( expected[ 2 ], (pvc->GetGestatingQueue()[ 2 ] + leaving_pvc->GetGestatingQueue()[ 2 ]) );

                CHECK_EQUAL(         pvc->GetNumGestating(), (        pvc->GetGestatingQueue()[1] +         pvc->GetGestatingQueue()[2]) );
                CHECK_EQUAL( leaving_pvc->GetNumGestating(), (leaving_pvc->GetGestatingQueue()[1] + leaving_pvc->GetGestatingQueue()[2]) );

                uint32_t leaving      = leaving_pvc->GetPopulation();
                uint32_t leaving_day2 = leaving_pvc->GetGestatingQueue()[ 1 ];
                uint32_t leaving_day3 = leaving_pvc->GetGestatingQueue()[ 2 ];
                uint32_t leaving_not  = leaving  - leaving_day2 - leaving_day3;
                //printf("%d,%d,%d,%d,%d,%d\n",num_leaving, leaving, total_leaving, leaving_day2,leaving_day3,leaving_not);

                CHECK_EQUAL( num_leaving, leaving );
                CHECK_EQUAL( (orig_population - num_leaving), pvc->GetPopulation() );

                total_leaving      += leaving;
                total_leaving_day2 += leaving_day2;
                total_leaving_day3 += leaving_day3;
                total_leaving_not  += leaving_not;

                pvc->Merge( leaving_pvc.get() );

                CHECK_EQUAL( exp_num_gestating, pvc->GetNumGestating() );
                CHECK_EQUAL( expected.size(), pvc->GetGestatingQueue().size() );
                CHECK_ARRAY_EQUAL( expected, pvc->GetGestatingQueue(), expected.size() );
            }
            float act_avg_leaving          = float( total_leaving ) / float( num_samples );
            float act_percent_leaving_day2 = float( total_leaving_day2 ) / float( num_samples ) / float( num_leaving );
            float act_percent_leaving_day3 = float( total_leaving_day3 ) / float( num_samples ) / float( num_leaving );
            float act_percent_leaving_not  = float( total_leaving_not  ) / float( num_samples ) / float( num_leaving );

            float exp_percent_leaving_day2 = float( num_day2 ) / float( pvc->GetPopulation() );
            float exp_percent_leaving_day3 = float( num_day3 ) / float( pvc->GetPopulation() );
            float exp_percent_leaving_not  = float( pvc->GetPopulation() - num_day2 - num_day3 ) / float( pvc->GetPopulation() );

            //printf("%d,%f,%f,%f,%f,%f,%f,%f\n",num_leaving,act_avg_leaving,
            //        exp_percent_leaving_day2, exp_percent_leaving_day3, exp_percent_leaving_not,
            //        act_percent_leaving_day2, act_percent_leaving_day3, act_percent_leaving_not );

            CHECK_CLOSE( float( num_leaving ),     act_avg_leaving,          0.03 );
            CHECK_CLOSE( exp_percent_leaving_day2, act_percent_leaving_day2, 0.03 );
            CHECK_CLOSE( exp_percent_leaving_day3, act_percent_leaving_day3, 0.03 );
            CHECK_CLOSE( exp_percent_leaving_not,  act_percent_leaving_not,  0.03 );
        }

    }

    TEST_FIXTURE( VectorCohortFixture, TestSplit_Distribution )
    {
        PSEUDO_DES rng( 2 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 0 );
        genome_self.SetLocus( 1, 1, 0 );
        genome_self.SetLocus( 2, 0, 1 );
        genome_self.SetLocus( 3, 1, 1 );

        VectorGenome genome_mate;
        genome_mate.SetLocus( 0, 0, 1 ); //male
        genome_mate.SetLocus( 1, 0, 1 );
        genome_mate.SetLocus( 2, 1, 1 );
        genome_mate.SetLocus( 3, 0, 0 );

        Stopwatch watch;
        watch.Start();

        std::vector<uint32_t> actual_gestating = { 0, 0, 0, 0 };
        uint32_t actual_non_gestating = 0;
        uint32_t actual_num_leaving = 0;
        uint32_t num_samples = 100000;
        uint32_t num_leaving = 1;
        uint32_t sample_pop = 10;
        std::vector<uint32_t> sample_gestating = { 1, 2, 3, 1 };
        uint32_t sample_total_gestating = 0;
        std::vector<float> percent_gestating;
        for( auto ges : sample_gestating )
        {
            sample_total_gestating += ges;
            percent_gestating.push_back( float(ges) / float(sample_pop) );
        }
        uint32_t sample_not_gestating = sample_pop - sample_total_gestating;
        float percent_not_gestating = float(sample_not_gestating) / float(sample_pop);

        for( uint32_t s = 0; s < num_samples; ++s )
        {
            std::unique_ptr<IVectorCohort> pvc( VectorCohort::CreateCohort( s,
                                                                            VectorStateEnum::STATE_ADULT,
                                                                            1.0f,
                                                                            0.0f,
                                                                            0.0f,
                                                                            sample_pop,
                                                                            genome_self,
                                                                            0 ) );

            pvc->SetMateGenome( genome_mate );
            CHECK_EQUAL( sample_pop, pvc->GetPopulation() );

            pvc->AddNewGestating( 1, sample_gestating[0] );
            pvc->AddNewGestating( 2, sample_gestating[1] );
            pvc->AddNewGestating( 3, sample_gestating[2] );
            pvc->AddNewGestating( 4, sample_gestating[3] );
            CHECK_EQUAL( sample_gestating.size(), pvc->GetGestatingQueue().size() );
            CHECK_ARRAY_EQUAL( sample_gestating, pvc->GetGestatingQueue(), sample_gestating.size() );
            CHECK_EQUAL( sample_not_gestating, (pvc->GetPopulation() - pvc->GetNumGestating()) );

            std::unique_ptr<IVectorCohort> split_pvc( pvc->SplitPercent( &rng, (num_samples+s), float(num_leaving)/ float( pvc->GetPopulation() ) ) );
            if( split_pvc.get() == nullptr ) continue;

            CHECK_EQUAL( actual_gestating.size(), split_pvc->GetGestatingQueue().size() );

            std::vector<uint32_t> queue = split_pvc->GetGestatingQueue();
            for( int i = 0; i < queue.size(); ++i )
            {
                actual_gestating[i] += queue[i];
            }
            actual_non_gestating += (split_pvc->GetPopulation() - split_pvc->GetNumGestating());
            actual_num_leaving += split_pvc->GetPopulation();
        }
        watch.Stop();
        double ms = watch.ResultNanoseconds() / 1000000.0;

        uint32_t expected_day   = num_leaving*num_samples;
        uint32_t expected_day_0 = uint32_t( percent_gestating[0]*float( num_leaving*num_samples ) );
        uint32_t expected_day_1 = uint32_t( percent_gestating[1]*float( num_leaving*num_samples ) );
        uint32_t expected_day_2 = uint32_t( percent_gestating[2]*float( num_leaving*num_samples ) );
        uint32_t expected_day_3 = uint32_t( percent_gestating[3]*float( num_leaving*num_samples ) );
        uint32_t expected_non   = uint32_t( percent_not_gestating*float( num_leaving*num_samples ) );

        float diff_day     = fabs( float( expected_day   ) - float( actual_num_leaving ) );
        float diff_day_0   = fabs( float( expected_day_0 ) - float( actual_gestating[ 0 ] ) );
        float diff_day_1   = fabs( float( expected_day_1 ) - float( actual_gestating[ 1 ] ) );
        float diff_day_2   = fabs( float( expected_day_2 ) - float( actual_gestating[ 2 ] ) );
        float diff_day_3   = fabs( float( expected_day_3 ) - float( actual_gestating[ 3 ] ) );
        float diff_day_non = fabs( float( expected_non   ) - float( actual_non_gestating  ) );

        float percent_diff_day   = (expected_day   == 0) ? 0.0f : fabs( diff_day     ) / float( expected_day   );
        float percent_diff_day_0 = (expected_day_0 == 0) ? 0.0f : fabs( diff_day_0   ) / float( expected_day_0 );
        float percent_diff_day_1 = (expected_day_1 == 0) ? 0.0f : fabs( diff_day_1   ) / float( expected_day_1 );
        float percent_diff_day_2 = (expected_day_2 == 0) ? 0.0f : fabs( diff_day_2   ) / float( expected_day_2 );
        float percent_diff_day_3 = (expected_day_3 == 0) ? 0.0f : fabs( diff_day_3   ) / float( expected_day_3 );
        float percent_diff_non   = (expected_non   == 0) ? 0.0f : fabs( diff_day_non ) / float( expected_non   );

        std::ostringstream msg;
        msg << "Dur (ms) = " << ms
            << "  day   = " << percent_diff_day
            << "  day_0 = " << percent_diff_day_0
            << "  day_1 = " << percent_diff_day_1
            << "  day_2 = " << percent_diff_day_2
            << "  day_3 = " << percent_diff_day_3
            << "  non,  = " << percent_diff_non
            << std::endl;
        PrintDebug( msg.str() );

        CHECK_CLOSE( 0.0, percent_diff_day  , 0.02 );
        CHECK_CLOSE( 0.0, percent_diff_day_0, 0.02 );
        CHECK_CLOSE( 0.0, percent_diff_day_1, 0.02 );
        CHECK_CLOSE( 0.0, percent_diff_day_2, 0.02 );
        CHECK_CLOSE( 0.0, percent_diff_day_3, 0.02 );
        CHECK_CLOSE( 0.0, percent_diff_non,   0.02 );
    }
}