/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "CalendarEventCoordinator.h"
#include "InterventionFactory.h"

SETUP_LOGGING( "CalendarEventCoordinator" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(CalendarEventCoordinator)

    IMPL_QUERY_INTERFACE2(CalendarEventCoordinator, IEventCoordinator, IConfigurable)

    CalendarEventCoordinator::CalendarEventCoordinator()
    : StandardInterventionDistributionEventCoordinator(false)//false=don't use standard demographic coverage
    {
    }

    bool
    CalendarEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        std::vector<int> distribution_times;
        std::vector<float> distribution_coverages;
        initConfigTypeMap("Distribution_Times", &distribution_times, Distribution_Times_DESC_TEXT, 1, INT_MAX, 0 );
        initConfigTypeMap("Distribution_Coverages", &distribution_coverages, Distribution_Coverages_DESC_TEXT, 0.0f, 1.0f, 0.0f );

        bool retValue = StandardInterventionDistributionEventCoordinator::Configure( inputJson );

        if( retValue && !JsonConfigurable::_dryrun )
        {
            if(distribution_times.size() != distribution_coverages.size())
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "In a Calendar Event Coordinator, vector of distribution coverages must match vector of distribution times" );
            }

            BuildDistributionCalendar(distribution_times, distribution_coverages);
        }
        return retValue;
    }

    void CalendarEventCoordinator::BuildDistributionCalendar(
        std::vector<int> distribution_times,
        std::vector<float> distribution_coverages
    )  
    {
        NaturalNumber last_time = 0;
        while (!distribution_times.empty())
        {
            NaturalNumber time = distribution_times.front();
            if( time == last_time )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ( std::string( "Duplicate distribution time entries: " ) + std::to_string( time ) ).c_str() );
            }
            else if( time < last_time )
            {
                std::stringstream msg;
                msg << "Distribution time mis-ordered: " << (int) last_time << " > " << (int) time << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
            Fraction coverage = distribution_coverages.front();
            distribution_times.erase( distribution_times.begin() );
            distribution_coverages.erase( distribution_coverages.begin() );
            times_and_coverages.insert( std::make_pair( time, coverage ) );
            last_time = time;
        }
    }

    void CalendarEventCoordinator::InitializeRepetitions( const Configuration* inputJson )
    {
        // don't include repetition parameters since they are not used
    }

    void CalendarEventCoordinator::UpdateRepetitions()
    {
        // do nothing for repetitions since they don't make sense for this coordinator
    }

    bool CalendarEventCoordinator::IsTimeToUpdate( float dt )
    {
        bool is_time_to_update = !distribution_complete;
        return is_time_to_update;
    }

    void CalendarEventCoordinator::UpdateNodes( float dt )
    {
        // Now issue events as they come up, including anything currently in the past or present
        while( parent->GetSimulationTime().time >= times_and_coverages.begin()->first)
        {
            StandardInterventionDistributionEventCoordinator::UpdateNodes( dt );

            times_and_coverages.erase(times_and_coverages.begin());
            LOG_DEBUG_F("%d Distributions remaining from CalendarEventCoordinator\n", times_and_coverages.size());
            if( times_and_coverages.empty() )
            {
                LOG_DEBUG_F("Signaling for disposal of CalendarEventCoordinator\n");
                distribution_complete = true; // we're done, signal disposal ok
                break;
            }
        }
    }

    bool
    CalendarEventCoordinator::TargetedIndividualIsCovered(
        IIndividualHumanEventContext *ihec
    )
    {
        auto coverage = times_and_coverages.begin()->second;
        if( coverage == 1.0 )
        {
            return true;
        }
        else if( coverage == 0.0 )
        {
            return false;
        }
        else
        {
            auto draw = randgen->e();
            LOG_DEBUG_F("randomDraw = %f, demographic_coverage = %f\n", draw, (float) coverage );
            return ( draw < coverage );
        }
    }
}
