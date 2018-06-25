/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVRandomChoice.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function

SETUP_LOGGING( "HIVRandomChoice" )

namespace Kernel
{
    const static float MIN_PROBABILITY = 0.0f ;
    const static float MAX_PROBABILITY = 1.0f ;

    // -----------------------------------------------------------------------------
    // --- This compare is used to sort the event list.  This is just to get around
    // --- change from using a map to a vector so that the regression tests still pass.
    // --- The vector will be faster and the features of a map are not used.
    // -----------------------------------------------------------------------------
    bool trigger_compare( const std::pair<EventTrigger, float>& pairA,
                          const std::pair<EventTrigger, float>& pairB )
    {
        return (pairA.first.ToString() < pairB.first.ToString());
    }

    void
    Event2ProbabilityType::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        EventTrigger event ;
        //event.parameter_name = "HIVRandomChoices::Choices::Event" ;

        float total = 0.0 ;
        const auto& tvcs_jo = json_cast<const json::Object&>( (*inputJson)[key] );
        for( auto data = tvcs_jo.Begin();
                  data != tvcs_jo.End();
                  ++data )
        {
            try {
                auto tvcs = inputJson->As< json::Object >()[ key ];

                event = data->name;
                float probability = 0.0f;
                try {
                    probability = (float) ((json::QuickInterpreter( tvcs ))[ data->name ].As<json::Number>());
                }
                catch( const json::Exception & )
                {
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, data->name.c_str(), (json::QuickInterpreter( tvcs )), "Expected NUMBER" );
                }

                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                // !!! BEWARE when duplicating this pattern.  Try to use the initConfig() pattern.
                // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                if ( probability > MAX_PROBABILITY )
                {
                    throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__,
                            "HIVRandomChoices::Choices::Probability", probability, MAX_PROBABILITY );
                }
                else if ( probability < MIN_PROBABILITY )
                {
                    throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__,
                            "HIVRandomChoices::Choices::Probability", probability, MIN_PROBABILITY );
                }

                event_list.push_back( std::make_pair( event, probability ) );

                total += probability ;
            }
            catch( const json::Exception & )
            {
                throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), (*inputJson), "Expected OBJECT" );
            }
        }

        if( total == 0.0 )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The sum of the probabilities in the 'Choices' table must be > 0." );
        }

        // normalize the probabilities
        for( auto& entry : event_list )
        {
            entry.second = entry.second / total ;
        }

        // See trigger_compare
        std::sort( event_list.begin(), event_list.end(), trigger_compare );
    }

    void Event2ProbabilityType::serialize( IArchive& ar, Event2ProbabilityType& mapping )
    {
        size_t count = ar.IsWriter() ? mapping.event_list.size() : -1;

        ar.startArray(count);
        if( ar.IsWriter() )
        {
            for (auto& entry : mapping.event_list)
            {
                std::string key   = entry.first.ToString();
                float value = entry.second;
                ar.startObject();
                    ar.labelElement("key"  ) & key;
                    ar.labelElement("value") & value;
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                std::string key;
                float value = 0.0;
                ar.startObject();
                    ar.labelElement("key"  ) & key;
                    ar.labelElement("value") & value;
                ar.endObject();

                EventTrigger event ;
                //event.parameter_name = "HIVRandomChoices::Choices::Event" ;
                event = key;

                mapping.event_list.push_back( std::make_pair( event,  value ) );
            }
        }
        ar.endArray();
    }

    json::QuickBuilder
    Event2ProbabilityType::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:Event2ProbabilityType" );

        schema[ts]["Event"] = json::Object();
        schema[ts]["Event"][ "type" ] = json::String( "Constrained String" );
        schema[ts]["Event"][ "description" ] = json::String( HIV_Event2ProbabilityMap_Event_DESC_TEXT );
        schema[ts]["Event"][ "value_source" ] = json::String( "<configuration>::Listed_Events.*" );
        schema[ts]["Probability"] = json::Object();
        schema[ts]["Probability"][ "type" ] = json::String( "float" );
        schema[ts]["Probability"][ "min" ] = json::Number( MIN_PROBABILITY );
        schema[ts]["Probability"][ "max" ] = json::Number( MAX_PROBABILITY );
        schema[ts]["Probability"][ "description" ] = json::String( HIV_Event2ProbabilityMap_Probability_DESC_TEXT );

        return schema;
    }
    BEGIN_QUERY_INTERFACE_DERIVED(HIVRandomChoice, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVRandomChoice, HIVSimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVRandomChoice)

    HIVRandomChoice::HIVRandomChoice()
    : HIVSimpleDiagnostic()
    {
        initSimTypes(1, "HIV_SIM" ); // just limiting this to HIV for release
    }

    HIVRandomChoice::HIVRandomChoice( const HIVRandomChoice& master )
        : HIVSimpleDiagnostic( master )
    {
        event2Probability = master.event2Probability;
    }

    bool HIVRandomChoice::Configure( const Configuration* inputJson )
    {
        initConfigComplexType("Choices", &event2Probability, HIV_Random_Choices_DESC_TEXT);

        return HIVSimpleDiagnostic::Configure( inputJson );
    }

    bool HIVRandomChoice::positiveTestResult()
    {
        return true;
    }

    void HIVRandomChoice::positiveTestDistribute()
    {
        LOG_DEBUG_F( "Individual %d tested HIVRandomChoice receiving actual intervention from HIVRandomChoice.\n", parent->GetSuid().data );

        // random number to choose an event from the dictionary
        float p = Environment::getInstance()->RNG->e();

        EventTrigger trigger;
        float probSum = 0;

        // pick the EventTrigger to broadcast
        for (auto ep : event2Probability.event_list)
        {
            probSum += ep.second;
            if (p <= probSum)
            {
                trigger = ep.first ;
                break;
            }
        }

        // expire the intervention
        expired = true;

        // broadcast the event
        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent->GetEventContext()->GetNodeEventContext()",
                                           "INodeTriggeredInterventionConsumer",
                                           "INodeEventContext" );
        }
        if( !trigger.IsUninitialized() )
        {
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), trigger );
        }
    }

    REGISTER_SERIALIZABLE(HIVRandomChoice);

    void HIVRandomChoice::serialize(IArchive& ar, HIVRandomChoice* obj)
    {
        HIVSimpleDiagnostic::serialize( ar, obj );
        HIVRandomChoice& choice = *obj;

        ar.labelElement("event2Probability") & choice.event2Probability;
    }
}
