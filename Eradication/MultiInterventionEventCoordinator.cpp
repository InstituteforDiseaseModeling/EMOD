/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MultiInterventionEventCoordinator.h"
#include "InterventionFactory.h"

SETUP_LOGGING( "MultiInterventionEventCoordinator" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(MultiInterventionEventCoordinator)

    IMPL_QUERY_INTERFACE2(MultiInterventionEventCoordinator, IEventCoordinator, IConfigurable)

    MultiInterventionEventCoordinator::MultiInterventionEventCoordinator()
    : StandardInterventionDistributionEventCoordinator()
    , m_IndividualInterventions()
    {
    }

    void MultiInterventionEventCoordinator::initializeInterventionConfig(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("Intervention_Configs", &intervention_config, MIEC_Intervention_Configs_DESC_TEXT );
    }

    void MultiInterventionEventCoordinator::validateInterventionConfig( const json::Element& rElement, const std::string& rDataLocation )
    {
        InterventionValidator::ValidateInterventionArray( intervention_config._json, rDataLocation );
    }

    bool MultiInterventionEventCoordinator::HasNodeLevelIntervention() const
    {
        bool has_node_level_intervention = false;
        bool has_individual_level_intervention = false;

        const json::Array & interventions_array = json::QuickInterpreter( intervention_config._json ).As<json::Array>();
        LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());
        for( int idx = 0; idx < interventions_array.Size(); idx++ )
        {
            const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[idx]);
            auto config = Configuration::CopyFromElement( actualIntervention, "campaign" );
            INodeDistributableIntervention *ndi = InterventionFactory::getInstance()->CreateNDIIntervention(config);
            if( ndi != nullptr )
            {
                has_node_level_intervention = true;
                ndi->Release();
            }
            else
            {
                has_individual_level_intervention = true;
            }
            delete config;
            config = nullptr;
        }

        if( has_node_level_intervention && has_individual_level_intervention )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "You cannot mix individual and nodel-level interventions." );
        }

        return has_node_level_intervention;
    }

    void MultiInterventionEventCoordinator::ExtractInterventionNameForLogging()
    {
        // Don't do anything
    }

    void MultiInterventionEventCoordinator::InitializeInterventions()
    {
        if( !has_node_level_intervention && m_IndividualInterventions.empty() )
        {
            const json::Array & interventions_array = json::QuickInterpreter( intervention_config._json ).As<json::Array>();
            LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());
            for( int idx = 0; idx < interventions_array.Size(); idx++ )
            {
                const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[ idx ]);
                Configuration * config = Configuration::CopyFromElement( actualIntervention );
                assert( config );

                LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", std::string( (*config)[ "class" ].As<json::String>() ).c_str() );
                IDistributableIntervention *di = InterventionFactory::getInstance()->CreateIntervention( config );
                assert( di );

                m_IndividualInterventions.push_back( di );

                delete config;
                config = nullptr;
            }
        }
    }

    void MultiInterventionEventCoordinator::DistributeInterventionsToNodes( INodeEventContext* event_context )
    {
        const json::Array & interventions_array = json::QuickInterpreter( intervention_config._json ).As<json::Array>();
        LOG_DEBUG_F( "interventions array size = %d\n", interventions_array.Size() );
        for( int idx = 0; idx < interventions_array.Size(); idx++ )
        {
            const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[ idx ]);
            Configuration * config = Configuration::CopyFromElement( actualIntervention );
            assert( config );

            LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n",
                std::string( (*config)[ "class" ].As<json::String>() ).c_str() );

            INodeDistributableIntervention *ndi = InterventionFactory::getInstance()->CreateNDIIntervention( config );
            if( ndi == nullptr )
            {
                throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "Should have constructed a node-level intervention." );
            }
            if( ndi->Distribute( event_context, this ) )
            {
                LOG_INFO_F( "UpdateNodes() distributed '%s' intervention to node %d\n", ndi->GetName().c_str(), event_context->GetId().data );
            }
            ndi->Release();

            delete config;
            config = nullptr;
        }
    }

    void MultiInterventionEventCoordinator::DistributeInterventionsToIndividuals( INodeEventContext* event_context )
    {
        // For now, distribute evenly across nodes. 
        int limitPerNode = -1;

        int totalIndivGivenIntervention = event_context->VisitIndividuals( this, limitPerNode );

        LOG_INFO_F( "UpdateNodes() gave out %d interventions at node %d\n", totalIndivGivenIntervention, event_context->GetId().data );
    }

    bool MultiInterventionEventCoordinator::DistributeInterventionsToIndividual( IIndividualHumanEventContext *ihec,
                                                                                 float & incrementalCostOut,
                                                                                 ICampaignCostObserver * pICCO )
    {
        bool all_distributed = true;
        for( auto di_master : m_IndividualInterventions )
        {
            // instantiate and distribute intervention
            LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", std::string( json::QuickInterpreter( intervention_config._json )[ "class" ].As<json::String>() ).c_str() );

            IDistributableIntervention* di_clone = di_master->Clone();
            release_assert( di_clone );
            di_clone->AddRef();

            if( !di_clone->Distribute( ihec->GetInterventionsContext(), pICCO ) )
            {
                di_clone->Release(); // a bit wasteful for now, could cache it for the next fellow
                LOG_DEBUG_F( "Distributed an intervention %s to individual %d at a cost of %f\n",
                        di_clone->GetName().c_str(), ihec->GetSuid().data, incrementalCostOut );
            }
            else
            {
                all_distributed = false;
            }
        }
        return all_distributed;
    }
}
