/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MultiInterventionEventCoordinator.h"
#include "InterventionFactory.h"

static const char * _module = "MultiInterventionEventCoordinator";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(MultiInterventionEventCoordinator)

    IMPL_QUERY_INTERFACE2(MultiInterventionEventCoordinator, IEventCoordinator, IConfigurable)

    MultiInterventionEventCoordinator::MultiInterventionEventCoordinator()
    : StandardInterventionDistributionEventCoordinator()
    {
    }

    void MultiInterventionEventCoordinator::initializeInterventionConfig(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("Intervention_Configs", &intervention_config, MIEC_Intervention_Configs_DESC_TEXT );
    }

    void MultiInterventionEventCoordinator::validateInterventionConfig( const json::Element& rElement )
    {
        InterventionValidator::ValidateInterventionArray( intervention_config._json );
    }

    bool MultiInterventionEventCoordinator::HasNodeLevelIntervention() const
    {
        bool has_node_level_intervention = false;

        const json::Array & interventions_array = json::QuickInterpreter( intervention_config._json ).As<json::Array>();
        LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());
        for( int idx = 0; !has_node_level_intervention && (idx < interventions_array.Size()); idx++ )
        {
            const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[idx]);
            auto qi_as_config = Configuration::CopyFromElement( actualIntervention );
            INodeDistributableIntervention *ndi = InterventionFactory::getInstance()->CreateNDIIntervention(qi_as_config);
            if( ndi != nullptr )
            {
                has_node_level_intervention = true;
                ndi->Release();
            }
            delete qi_as_config;
            qi_as_config = nullptr;
        }

        return has_node_level_intervention;
    }


    // copy/paste to remove single-intervention-specific logging
    // TODO: could be handled more gracefully
    // also not robust in case of array containing NTI not just ITI
    void MultiInterventionEventCoordinator::UpdateNodes( float dt )
    {
        // Only call VisitNodes on first call and if countdown == 0
        if( tsteps_since_last != tsteps_between_reps )
        {
            return;
        }

        int grandTotal = 0;
        int limitPerNode = -1;

        LOG_DEBUG_F("[UpdateNodes] limitPerNode = %d\n", limitPerNode);
        for (auto nec : cached_nodes)
        {
            try
            {
                // For now, distribute evenly across nodes. 
                int totalIndivGivenIntervention = nec->VisitIndividuals( this, limitPerNode );
                grandTotal += totalIndivGivenIntervention;
                LOG_INFO_F( "UpdateNodes() gave out %d interventions at node %d\n", totalIndivGivenIntervention, nec->GetId().data );
            }
            catch(json::Exception &e)
            {
                // ERROR: not ITI???
                // ERROR: ::cerr << "exception casting intervention_config to array! " << e.what() << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); // ( "Intervention_Configs json problem: intervention_config is valid json but needs to be an array." );
            }
        }

        tsteps_since_last = 0;
        num_repetitions--;
        if( num_repetitions == 0 )
        {
            distribution_complete = true; // we're done, signal disposal ok
        }
    }

    bool
    MultiInterventionEventCoordinator::visitIndividualCallback( 
        IIndividualHumanEventContext *ihec,
        float & incrementalCostOut,
        ICampaignCostObserver * pICCO
    )
    {
        // Less of this would need to be copied from the base class with a more thoughtful encapsulation of functions
        // In particular, only the give-intervention(s)-to-individual stuff inside the try statement is different.
        if( qualifiesDemographically( ihec ) == false )
        {
            LOG_DEBUG("Individual not given intervention because not in target demographic\n");
            return false;
        }
        LOG_DEBUG("Individual meets demographic targeting criteria\n");

        if (!TargetedIndividualIsCovered(ihec))
        {
            incrementalCostOut = 0;
            return false;
        }
        else
        {
            incrementalCostOut = 0;

            try
            {
                const json::Array & interventions_array = json::QuickInterpreter( intervention_config._json ).As<json::Array>();
                LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());
                for( int idx=0; idx<interventions_array.Size(); idx++ )
                {
                    const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[idx]);
                    Configuration * tmpConfig = Configuration::CopyFromElement(actualIntervention);
                    assert( tmpConfig );

                    // instantiate and distribute intervention
                    LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", std::string((*tmpConfig)["class"].As<json::String>()).c_str() );
                    IDistributableIntervention *di = InterventionFactory::getInstance()->CreateIntervention(tmpConfig);
                    assert(di);
                    delete tmpConfig;
                    tmpConfig = nullptr;
                    if (di)
                    {
                        if (!di->Distribute( ihec->GetInterventionsContext(), pICCO ) )
                        {
                            di->Release(); // a bit wasteful for now, could cache it for the next fellow
                        }

                        LOG_DEBUG_F("Distributed an intervention %p to individual %d at a cost of %f\n", di, ihec->GetSuid().data, incrementalCostOut);
                    }
                }
            }
            catch(json::Exception &e)
            {
                // ERROR: ::cerr << "exception casting intervention_config to array! " << e.what() << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); // ( "InterventionConfigs json problem: intervention_config is valid json but needs to be an array." );
            }

        }
        return true;
    }
}
