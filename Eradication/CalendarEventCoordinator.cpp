/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "CalendarEventCoordinator.h"
#include "InterventionFactory.h"

static const char * _module = "CalendarEventCoordinator";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(CalendarEventCoordinator)

    IMPL_QUERY_INTERFACE2(CalendarEventCoordinator, IEventCoordinator, IConfigurable)

    CalendarEventCoordinator::CalendarEventCoordinator()
    : StandardInterventionDistributionEventCoordinator()
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

        if(distribution_times.size() != distribution_coverages.size())
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "In a Calendar Event Coordinator, vector of distribution coverages must match vector of distribution times" );
        }

        BuildDistributionCalendar(distribution_times, distribution_coverages);

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

    void CalendarEventCoordinator::UpdateNodes( float dt )
    {
        // Now issue events as they come up, including anything currently in the past or present
        while( parent->GetSimulationTime().time >= times_and_coverages.begin()->first)
        {
            int grandTotal = 0;
            int limitPerNode = -1;

            // intervention class names for informative logging
            std::ostringstream intervention_name;
            intervention_name << std::string( json::QuickInterpreter(intervention_config._json)["class"].As<json::String>() );

            auto qi_as_config = Configuration::CopyFromElement( (intervention_config._json) );
            _di = InterventionFactory::getInstance()->CreateIntervention(qi_as_config);
            // including deeper information for "distributing" interventions (e.g. calendars)
            formatInterventionClassNames( intervention_name, &json::QuickInterpreter(intervention_config._json) );

            // Only visit individuals if this is NOT an NTI. Check...
            // Check to see if intervention is an INodeDistributable...
            INodeDistributableIntervention *ndi = InterventionFactory::getInstance()->CreateNDIIntervention(qi_as_config);
            INodeDistributableIntervention *ndi2 = nullptr;

            LOG_DEBUG_F("[UpdateNodes] limitPerNode = %d\n", limitPerNode);
            for (auto nec : cached_nodes)
            {
                if (ndi)
                {
                    throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__ );
#if 0
                    ndi2 = InterventionFactory::getInstance()->CreateNDIIntervention( qi_as_config );
                    if(ndi2)
                    {
                        float duration = -1;
                        if (times_and_coverages.size() > 1)
                        {
                            auto iter = times_and_coverages.end(); 
                            //A node-targeted intervention issued through the calender coordinator lasts until the next NDI.  
                            //Is there an overlap of one day here?  Should there be a -1?
                            duration = (float)(prev(iter,2)->first - prev(iter, 1)->first);   
                        }
                        
                        INodeDistributableInterventionParameterSetterInterface* pNDIPSI = nullptr;
                        if (s_OK == ndi2->QueryInterface(GET_IID(INodeDistributableInterventionParameterSetterInterface), (void**)&pNDIPSI) )
                        {
                            pNDIPSI->SetDemographicCoverage(times_and_coverages.begin()->second);
                            pNDIPSI->SetMaxDuration(duration);
                        }
                                                
                        if (!ndi2->Distribute( nec, this ) )
                        {
                            LOG_INFO_F("UpdateNodes() distributed '%s' intervention to node %d\n", intervention_name.str().c_str(), nec->GetId().data );
                        }
                        ndi2->Release();
                    }
#endif
                }
                else
                {
                    try
                    {
                        // For now, distribute evenly across nodes. 
                        int totalIndivGivenIntervention = nec->VisitIndividuals( this, limitPerNode );
                        grandTotal += totalIndivGivenIntervention;
                        LOG_INFO_F( "UpdateNodes() gave out %d interventions at node %d\n", totalIndivGivenIntervention, nec->GetId().data );
                    }                   
                    catch( const json::Exception &e )
                    {
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() );
                    }
                }
            }
            delete qi_as_config;
            qi_as_config = nullptr;

            times_and_coverages.erase(times_and_coverages.begin());
            LOG_DEBUG_F("%d Distributions remaining from CalendarEventCoordinator\n", times_and_coverages.size());
            if( times_and_coverages.empty() )
            {
                LOG_DEBUG_F("Signaling for disposal of CalendarEventCoordinator\n");
                distribution_complete = true; // we're done, signal disposal ok
                break;
            }
        }
        return;
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
