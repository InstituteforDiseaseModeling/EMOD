/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include <stdafx.h>
#ifndef WIN32
#include <cxxabi.h>
#endif

#include "Interventions.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"
#include "Contexts.h"
#include "Exceptions.h"

#include "RapidJsonImpl.h"

static const char* _module = "Interventions";

//unsigned int total_intervention_counter = 0;

namespace Kernel
{
    void CheckSimType( const std::string& rSimTypeStr,
                       const json::Object& rJsonObject,
                       const char* pClassName )
    {
        json::QuickInterpreter sim_type_qi( rJsonObject[ "Sim_Types" ] );
        json::QuickInterpreter sim_type_array( sim_type_qi.As<json::Array>() );

        int num_types = sim_type_qi.As<json::Array>().Size() ;
        assert( num_types > 0 );

        if( std::string(sim_type_array[0].As<json::String>()) == std::string("*") )
        {
            // wild card means it is valid for any simulation type
            return ;
        }

        std::string supported ;
        for( int i = 0 ; i < num_types ; i++ )
        {
            std::string supported_sim_type = std::string(sim_type_array[i].As<json::String>()) ;
            if( rSimTypeStr == supported_sim_type )
            {
                return ;
            }
            supported += "'"+supported_sim_type + "', " ;
        }
        supported = supported.substr( 0, supported.length() - 2 );

        std::stringstream ss ;
        ss << "The '" << pClassName << "' intervention is not valid with the current 'Simulation_Type' (='" << rSimTypeStr << "').  " ;
        ss << "This intervention is only supported for the following simulation types: " << supported ;
        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }

    BaseIntervention::BaseIntervention()
        : name( typeid(*this).name() )
        , cost_per_unit(0.0f)
        , expired(false)
        , dont_allow_duplicates(false)
    {
#ifndef WIN32
        name = abi::__cxa_demangle(name.c_str(), 0, 0, nullptr );
#endif

        //total_intervention_counter++;
        initSimTypes( 1, "*" );
        initConfigTypeMap( "Dont_Allow_Duplicates", &dont_allow_duplicates, Dont_Allow_Duplicates_DESC_TEXT, false );
        initConfigTypeMap( "Intervention_Name", &name, Intervention_Name_DESC_TEXT, typeid(*this).name() );
        //LOG_DEBUG_F("New intervention, total_intervention_counter = %d\n", total_intervention_counter);
    }

    BaseIntervention::BaseIntervention( const BaseIntervention& master )
    {
        name                  = master.name;
        cost_per_unit         = master.cost_per_unit;
        expired               = master.expired;
        dont_allow_duplicates = master.dont_allow_duplicates ;
    }

    BaseIntervention::~BaseIntervention()
    {
        //total_intervention_counter--;
        //LOG_DEBUG_F("Deleted intervention, total_intervention_counter = %d\n", total_intervention_counter);
    }

    bool
    BaseIntervention::Expired()
    {
        return expired;
    }

    void
    BaseIntervention::ValidateSimType( 
        const std::string& rSimTypeStr
    )
    {
        CheckSimType( rSimTypeStr, jsonSchemaBase, typeid(*this).name() );
    }

    bool
    BaseIntervention::Distribute(
        IIndividualHumanInterventionsContext *context, // interventions container usually
        ICampaignCostObserver * const pICCO
    )
    {
        if( dont_allow_duplicates && context->ContainsExisting( typeid(*this).name() ) )
        {
            return false ;
        }

        bool wasDistributed=false;
        IInterventionConsumer * ic;
        if (s_OK == (context->QueryInterface(GET_IID(IInterventionConsumer), (void**)&ic) ) )
        {
            assert(ic);
            if( ic->GiveIntervention(this) )
            {
                // Need to get Individual pointer from interventions container pointer. Try parent.
                IIndividualHumanEventContext * pIndiv = (context->GetParent())->GetEventContext();
                if( pICCO )
                {
                    pICCO->notifyCampaignExpenseIncurred( cost_per_unit, pIndiv );
                }
                wasDistributed = true;
            }
        } 
        else
        {
            std::ostringstream oss;
            oss << "Unable to distribute intervention because IIndividualHumanInterventionsContext doesn't support IInterventionConsumer." << std::endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
        }
        return wasDistributed;
    }

    // BaseNodeIntervention
    BaseNodeIntervention::BaseNodeIntervention()
        : name( typeid(*this).name() )
        , cost_per_unit(0.0f)
        , expired(false)
    {
#ifndef WIN32
        name = abi::__cxa_demangle(name.c_str(), 0, 0, nullptr );
#endif
        initSimTypes( 1, "*" );
        initConfigTypeMap( "Intervention_Name", &name, Intervention_Name_DESC_TEXT, typeid(*this).name() );
        LOG_DEBUG_F("New intervention, cost_per_unit = %f\n", cost_per_unit);
    }

    bool
    BaseNodeIntervention::Expired()
    {
        return expired;
    }

    void
    BaseNodeIntervention::ValidateSimType( 
        const std::string& rSimTypeStr
    )
    {
        CheckSimType( rSimTypeStr, jsonSchemaBase, typeid(*this).name() );
    }

    bool
    BaseNodeIntervention::Distribute(
        INodeEventContext *context, // interventions container usually
        IEventCoordinator2 * ec
    )
    {
        bool wasDistributed=false;
        INodeInterventionConsumer * ic;
        if (s_OK == (context->QueryInterface(GET_IID(INodeInterventionConsumer), (void**)&ic) ) )
        {
            assert(ic);
            if( ic->GiveIntervention(this) )
            {
                wasDistributed = true;
            }
        } 
        else
        {
            std::ostringstream oss;
            oss << "Unable to distribute intervention because INodeEventContext doesn't support INodeInterventionConsumer." << std::endl;
            throw IllegalOperationException(  __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
        }
        return wasDistributed;
    }

    void BaseIntervention::serialize(IArchive& ar, BaseIntervention* obj)
    {
        BaseIntervention& intervention = *obj;
        ar.labelElement("name"                 ) & intervention.name;
        ar.labelElement("cost_per_unit"        ) & intervention.cost_per_unit;
        ar.labelElement("expired"              ) & intervention.expired;
        ar.labelElement("dont_allow_duplicates") & intervention.dont_allow_duplicates;
    }
}
