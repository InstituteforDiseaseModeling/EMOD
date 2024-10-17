
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

SUITE( VectorCohortMaleTest )
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
        genome_self.SetLocus( 0, 0, 1 ); //male
        genome_self.SetLocus( 1, 0, 1 );
        genome_self.SetLocus( 2, 1, 1 );
        genome_self.SetLocus( 3, 0, 0 );
        genome_self.SetWolbachia( VectorWolbachia::VECTOR_WOLBACHIA_AB );

        std::unique_ptr<VectorCohortMale> pvc( VectorCohortMale::CreateCohort( 1,
                                                                               1.0f,
                                                                               0.1f,
                                                                               7.0,
                                                                               99,
                                                                               genome_self,
                                                                               0 ) );

        CHECK_EQUAL( VectorStateEnum::STATE_MALE, pvc->GetState() );
        CHECK_EQUAL( 1.0f, pvc->GetAge() );
        CHECK_EQUAL( 0.1f, pvc->GetProgress() );
        CHECK_EQUAL( 99, pvc->GetPopulation() );
        CHECK_EQUAL( 99, pvc->GetUnmatedCount() );
        CHECK_EQUAL( 0, pvc->GetUnmatedCountCDF() );
        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        CHECK_EQUAL( genome_empty.GetBits(), pvc->GetMateGenome().GetBits() );
       // CHECK_EQUAL( 0, pvc->RemoveNumDoneGestating() ); add as error tests
       // CHECK_EQUAL( 0, pvc->GetGestatingQueue().size() ); add as error tests
        CHECK_EQUAL( VectorWolbachia::VECTOR_WOLBACHIA_AB, pvc->GetWolbachia() );
        CHECK_EQUAL( 0, pvc->GetIMigrate()->GetMigrationDestination().data );
        CHECK_EQUAL( MigrationType::NO_MIGRATION, pvc->GetIMigrate()->GetMigrationType() );
        CHECK_EQUAL( 7.0, pvc->GetDurationOfMicrosporidia() );

       // pvc->SetState( VectorStateEnum::STATE_ADULT );add as error tests
       // CHECK_EQUAL( VectorStateEnum::STATE_ADULT, pvc->GetState() );

        pvc->SetAge( 3.0 );
        CHECK_EQUAL( 3.0, pvc->GetAge() );

        pvc->IncreaseAge( 1.0 );
        CHECK_EQUAL( 4.0, pvc->GetAge() );

        pvc->SetPopulation( 77 );
        CHECK_EQUAL( 77, pvc->GetPopulation() );

        pvc->SetUnmatedCount(88) ;
        CHECK_EQUAL( 88, pvc->GetUnmatedCount() );

        pvc->SetUnmatedCountCDF( 234 );
        CHECK_EQUAL( 234, pvc->GetUnmatedCountCDF() );

       // pvc->SetMateGenome( genome_mate ); add as error tests
        //CHECK_EQUAL( genome_mate.GetBits(), pvc->GetMateGenome().GetBits() );

        CHECK_EQUAL( 0.1f, pvc->GetProgress() );
        pvc->ClearProgress( );
        CHECK_EQUAL( 0.0f, pvc->GetProgress() );

        suids::suid id;
        id.data = 3;
        pvc->GetIMigrate()->SetMigrating( id, MigrationType::LOCAL_MIGRATION, 0.0, 0.0, false );
        CHECK_EQUAL( 3, pvc->GetIMigrate()->GetMigrationDestination().data );
        CHECK_EQUAL( MigrationType::LOCAL_MIGRATION, pvc->GetIMigrate()->GetMigrationType() );
    }

    TEST_FIXTURE( VectorCohortFixture, TestSplitPercent )
    {
        PSEUDO_DES rng( 2 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 1 ); //male
        genome_self.SetLocus( 1, 0, 1 );
        genome_self.SetLocus( 2, 1, 1 );
        genome_self.SetLocus( 3, 0, 0 );

        std::unique_ptr<VectorCohortMale> pvc( VectorCohortMale::CreateCohort( 1,
                                                                               1.0f,
                                                                               0.0f,
                                                                               0.0f,
                                                                               100,
                                                                               genome_self,
                                                                               0 ) );
        pvc->SetUnmatedCountCDF(294);
        CHECK_EQUAL( 294, pvc->GetUnmatedCountCDF() );
        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        CHECK_EQUAL( 100, pvc->GetPopulation() );
        CHECK_EQUAL( 100, pvc->GetUnmatedCount() );
        pvc->SetUnmatedCount( 87 );
        CHECK_EQUAL( 87,  pvc->GetUnmatedCount() );


        std::unique_ptr<VectorCohortMale> split_pvc( pvc->SplitPercent( &rng, 2, 0.4f ) );

        CHECK_EQUAL( genome_self.GetBits(), split_pvc->GetGenome().GetBits() );
        CHECK_EQUAL( 43, split_pvc->GetPopulation() );
        CHECK_EQUAL( 36, split_pvc->GetUnmatedCount() );
        CHECK_EQUAL( 0,  split_pvc->GetUnmatedCountCDF() );


        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        CHECK_EQUAL( 57,  pvc->GetPopulation() );
        CHECK_EQUAL( 51,  pvc->GetUnmatedCount() );
        CHECK_EQUAL( 294, pvc->GetUnmatedCountCDF() );


        pvc->Merge( split_pvc.get() );

        CHECK_EQUAL( 100, pvc->GetPopulation() );
        CHECK_EQUAL( 87,  pvc->GetUnmatedCount() );
        CHECK_EQUAL( 294, pvc->GetUnmatedCountCDF() );


        // ---------------------------------------------------------------------------
        // --- Test that SplitPercent() using percentage works when no vectors are unmated
        // ---------------------------------------------------------------------------
        pvc->SetUnmatedCount( 0 );
        pvc->SetUnmatedCountCDF( 0 );
        CHECK_EQUAL( 100, pvc->GetPopulation() );
        CHECK_EQUAL( 0,   pvc->GetUnmatedCount() );
        CHECK_EQUAL( 0,   pvc->GetUnmatedCountCDF() );

        std::unique_ptr<VectorCohortMale> split_pvc3( pvc->SplitPercent( &rng, 3, 0.5 ) );

        CHECK_EQUAL( 43, split_pvc3->GetPopulation() );
        CHECK_EQUAL( 0,  split_pvc3->GetUnmatedCount() );
        CHECK_EQUAL( 0,  split_pvc3->GetUnmatedCountCDF() );
        CHECK_EQUAL( 57, pvc->GetPopulation() );
        CHECK_EQUAL( 0,  pvc->GetUnmatedCount() );
        CHECK_EQUAL( 0,  pvc->GetUnmatedCountCDF() );

        // ---------------------------------------------------------------------------
        // --- Test that SplitPercent() using percentage works when percentage is very small
        // --- It will return nullptr because binomial_approx() will return 0.
        // ---------------------------------------------------------------------------
        std::unique_ptr<VectorCohortMale> pvc9( VectorCohortMale::CreateCohort( 9,
                                                                                1.0f,
                                                                                0.0f,
                                                                                0.0f,
                                                                                1000000,
                                                                                genome_self,
                                                                                0 ) );

        CHECK_EQUAL( 1000000, pvc9->GetPopulation() );

        VectorCohortMale* p_expect_to_be_null = pvc9->SplitPercent( &rng, 10, 0.000001f );

        CHECK_EQUAL( ( VectorCohortMale*)nullptr, p_expect_to_be_null );


        // -------------------------------------------------------------------------------
        // --- Test SplitPercent() that correct proportion of unmated leaving as well. 
        // -------------------------------------------------------------------------------

        uint32_t orig_population = 300;
        uint32_t orig_unmated = 200;

        uint32_t num_samples = 100000;
        float percent_leaving = 0.1f; // 30 vectors, 20 of them unmated on average
        uint32_t total_unmated_leaving = 0;
        uint32_t total_leaving = 0;
        uint32_t total_unmated_stayed = 0;
        uint32_t total_stayed = 0;
        for( uint32_t i = 0; i < num_samples; ++i )
        {
            pvc->SetPopulation( orig_population ); //resetting for the next split
            pvc->SetUnmatedCount( orig_unmated );
            std::unique_ptr<VectorCohortMale> leaving_pvc( pvc->SplitPercent( &rng, 5, percent_leaving ) );

            CHECK( pvc->GetPopulation() >= pvc->GetUnmatedCount() );
            CHECK( leaving_pvc->GetPopulation() >= leaving_pvc->GetUnmatedCount() );

            total_unmated_stayed += pvc->GetUnmatedCount();
            total_stayed += pvc->GetPopulation();
            total_unmated_leaving += leaving_pvc->GetUnmatedCount();
            total_leaving += leaving_pvc->GetPopulation();
        }

        float act_avg_leaving = float( total_leaving ) / float( num_samples );
        float act_avg_unmated_leaving = float( total_unmated_leaving ) / float( num_samples );
        float act_avg_stayed = float( total_stayed ) / float( num_samples );
        float act_avg_unmated_stayed = float( total_unmated_stayed ) / float( num_samples );

        float act_percent_leaving = float( act_avg_leaving ) / float( orig_population );
        float act_percent_unmated_leaving = float( act_avg_unmated_leaving ) / float( orig_unmated );
        float act_percent_stayed = float( act_avg_stayed ) / float( orig_population );
        float act_percent_unmated_stayed= float( act_avg_unmated_stayed ) / float( orig_unmated );

        CHECK_CLOSE( float( percent_leaving * orig_population ), act_avg_leaving, 1 );
        CHECK_CLOSE( float( percent_leaving * orig_unmated ), act_avg_unmated_leaving, 1 );
        CHECK_CLOSE( percent_leaving , act_percent_leaving, 0.03 );
        CHECK_CLOSE( percent_leaving, act_percent_unmated_leaving, 0.03 );
        CHECK_CLOSE( 1.0 - percent_leaving, act_percent_stayed, 0.03 );
        CHECK_CLOSE( 1.0 - percent_leaving, act_percent_unmated_stayed, 0.03 );
    }

    TEST_FIXTURE( VectorCohortFixture, TestSplitNumber )
    {
        PSEUDO_DES rng( 2 );

        VectorGenome genome_self;
        genome_self.SetLocus( 0, 0, 1 ); //male
        genome_self.SetLocus( 1, 0, 1 );
        genome_self.SetLocus( 2, 1, 1 );
        genome_self.SetLocus( 3, 0, 0 );

        std::unique_ptr<VectorCohortMale> pvc( VectorCohortMale::CreateCohort( 1,
                                                                               1.0f,
                                                                               0.0f,
                                                                               0.0f,
                                                                               100,
                                                                               genome_self,
                                                                               0 ) );

        CHECK_EQUAL( genome_self.GetBits(), pvc->GetGenome().GetBits() );
        pvc->SetUnmatedCountCDF( 215 );
        CHECK_EQUAL( 100, pvc->GetPopulation() );

        // ----------------------------------
        // --- Test SplitNumber() using num leaving
        // ----------------------------------

        std::unique_ptr<VectorCohortMale> split_pvc2( pvc->SplitNumber( &rng, 2, 13 ) );

        CHECK_EQUAL( 13, split_pvc2->GetPopulation() );
        CHECK_EQUAL( 13, split_pvc2->GetUnmatedCount() );
        CHECK_EQUAL( 0,  split_pvc2->GetUnmatedCountCDF() );
        CHECK_EQUAL( 87,        pvc->GetPopulation() );
        CHECK_EQUAL( 87,        pvc->GetUnmatedCount() );
        CHECK_EQUAL( 215,       pvc->GetUnmatedCountCDF() );


        pvc->Merge( split_pvc2.get() );

        CHECK_EQUAL( 100, pvc->GetPopulation() );
        CHECK_EQUAL( 100, pvc->GetUnmatedCount() );
        CHECK_EQUAL( 215, pvc->GetUnmatedCountCDF() );

        // --------------------------------------------------------------------
        // --- Test SplitNumber() using num leaving works when not all unmated
        // --------------------------------------------------------------------
        pvc->SetUnmatedCountCDF( 684 );
        pvc->SetUnmatedCount( 12 );

        std::unique_ptr<VectorCohortMale> split_pvc5( pvc->SplitNumber( &rng, 5, 43 ) );

        CHECK_EQUAL( 43, split_pvc5->GetPopulation() );
        CHECK_EQUAL( 4,  split_pvc5->GetUnmatedCount() );
        CHECK_EQUAL( 0,  split_pvc5->GetUnmatedCountCDF() );
        CHECK_EQUAL( 57,        pvc->GetPopulation() );
        CHECK_EQUAL( 8,         pvc->GetUnmatedCount() );
        CHECK_EQUAL( 684,       pvc->GetUnmatedCountCDF() );


        pvc->Merge( split_pvc5.get() );
        CHECK_EQUAL( 100, pvc->GetPopulation() );
        CHECK_EQUAL( 12,  pvc->GetUnmatedCount() );
        CHECK_EQUAL( 684, pvc->GetUnmatedCountCDF() );


        // -------------------------------------------------------------------------------
        // --- Test SplitNumber() that correct proportion of unmated leaving as well. 
        // -------------------------------------------------------------------------------

        uint32_t orig_population = 300;
        uint32_t orig_unmated = 200;

        uint32_t num_samples = 100000;
        uint32_t num_leaving = 30; // expect about 0.1  (20ish) of unmated to leave as well
        uint32_t total_unmated_leaving = 0;
        uint32_t total_leaving = 0;
        uint32_t total_unmated_stayed = 0;
        uint32_t total_stayed = 0;
        for( uint32_t i = 0; i < num_samples; ++i )
        {
            pvc->SetPopulation( orig_population ); //resetting for the next split
            pvc->SetUnmatedCount( orig_unmated );
            std::unique_ptr<VectorCohortMale> leaving_pvc( pvc->SplitNumber( &rng, 5, num_leaving ) );

            CHECK(         pvc->GetPopulation() >=         pvc->GetUnmatedCount() );
            CHECK( leaving_pvc->GetPopulation() >= leaving_pvc->GetUnmatedCount() );

            total_unmated_stayed += pvc->GetUnmatedCount();
            total_stayed         += pvc->GetPopulation();
            total_unmated_leaving += leaving_pvc->GetUnmatedCount();
            total_leaving        += leaving_pvc->GetPopulation();
        }

        float act_avg_leaving = float( total_leaving ) / float( num_samples );
        float act_avg_unmated_leaving = float( total_unmated_leaving ) / float( num_samples );
        float act_avg_stayed = float( total_stayed ) / float( num_samples );
        float act_avg_unmated_stayed = float( total_unmated_stayed ) / float( num_samples );

        float exp_percent_leaving = float( num_leaving ) / float( orig_population );
        float exp_percent_stayed = 1.0 - exp_percent_leaving;

        CHECK_CLOSE( float( num_leaving ), act_avg_leaving, 1 );
        CHECK_CLOSE( float( exp_percent_leaving * orig_unmated ), act_avg_unmated_leaving, 1);
        CHECK_CLOSE( exp_percent_leaving, act_avg_leaving / float( orig_population ), 0.03);
        CHECK_CLOSE( exp_percent_leaving, act_avg_unmated_leaving/ float( orig_unmated ), 0.03);
        CHECK_CLOSE( exp_percent_stayed, act_avg_stayed / float( orig_population ), 0.03 );
        CHECK_CLOSE( exp_percent_stayed, act_avg_unmated_stayed / float( orig_unmated ), 0.03 );

    }

}