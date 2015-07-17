/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include <stdafx.h>
#include "Interventions.h"
#include "InterventionFactory.h"
#include "IndividualEventContext.h"
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
        : cost_per_unit(0.0f)
        , expired(false)
    {
        //total_intervention_counter++;
        initSimTypes( 1, "*" );
        //LOG_DEBUG_F("New intervention, total_intervention_counter = %d\n", total_intervention_counter);
    }

    BaseIntervention::BaseIntervention( const BaseIntervention& master )
    {
        cost_per_unit = master.cost_per_unit;
        expired = master.expired;
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
        : cost_per_unit(0.0f)
        , expired(false)
    {
        initSimTypes( 1, "*" );
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

#if USE_JSON_SERIALIZATION || USE_JSON_MPI

    // IJsonSerializable Interfaces
    void BaseIntervention::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        // begin json object
        root->BeginObject();

        root->Insert("cost_per_unit",cost_per_unit);
        root->Insert("expired", expired);

        root->EndObject();
    }

    void BaseIntervention::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {

        LOG_INFO( "In JDeserialize\n");
        rapidjson::Document * doc = (rapidjson::Document*) root;   
        cost_per_unit = (*doc)["cost_per_unit"].GetDouble();
        expired = (*doc)["expired"].GetBool();

    }

    // IJsonSerializable Interfaces
    void BaseNodeIntervention::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("cost_per_unit",cost_per_unit);
        root->Insert("expired", expired);

        root->EndObject();
    }

    void BaseNodeIntervention::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif

}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::BaseIntervention)

namespace Kernel {

    template<class Archive>
    void serialize(Archive &ar, BaseIntervention &bi, const unsigned int v)
    {
        ar & bi.cost_per_unit;
        ar & bi.expired;
    }
}
#endif
