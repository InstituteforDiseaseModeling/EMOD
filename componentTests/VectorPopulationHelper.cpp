
#include "stdafx.h"
#include "VectorPopulationHelper.h"

#include "UnitTest++.h"
#include "IVectorCohort.h"
#include "VectorCohortCollection.h"

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

void CheckQueueInitialization( bool isFemale,
                               uint32_t expectedNumPop,
                               uint32_t daysBetweenFeeds,
                               const Kernel::VectorCohortCollectionAbstract& rQueue )
{
    uint32_t total = 0;
    std::map<float, std::vector<Kernel::IVectorCohort*>> age_map;
    for( auto pvc : rQueue )
    {
        total += pvc->GetPopulation();
        age_map[ pvc->GetAge() ].push_back( pvc );
    }
    CHECK_EQUAL( expectedNumPop, total );
    CHECK_EQUAL( daysBetweenFeeds, age_map.size() );

    float expected_age = 1.0;
    for( auto entry : age_map )
    {
        int num_at_age = 0;
        std::vector<uint32_t> total_gestating_queue( daysBetweenFeeds, 0 );
        for( auto pvc : entry.second )
        {
            if( isFemale )
            {
                pvc->ReportOnGestatingQueue( total_gestating_queue );

            }
            num_at_age += pvc->GetPopulation();
        }

        if( isFemale )
        {
            uint32_t index = daysBetweenFeeds - uint32_t( expected_age );
            for( uint32_t i = 0; i < daysBetweenFeeds; ++i )
            {
                if( index == i )
                {
                    CHECK_CLOSE( int( expectedNumPop / daysBetweenFeeds ), int( total_gestating_queue[ i ] ), int( 200 ) );
                }
                else
                {
                    CHECK_EQUAL( 0, total_gestating_queue[ i ] );
                }
            }
        }

        CHECK_CLOSE( int( expectedNumPop / daysBetweenFeeds ), num_at_age, 200 );
        CHECK_EQUAL( expected_age, entry.first );
        ++expected_age;
    }
}

