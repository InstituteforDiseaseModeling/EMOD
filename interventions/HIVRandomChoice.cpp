/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVRandomChoice.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function and access into IHIVCascadeOfCare

static const char * _module = "HIVRandomChoice";

namespace Kernel
{
    const static float MIN_PROBABILITY = 0.0f ;
    const static float MAX_PROBABILITY = 1.0f ;

    void
    Event2ProbabilityMapType::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        EventTrigger event ;
        event.parameter_name = "HIVRandomChoices::Choices::Event" ;

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

                (this)->insert( std::make_pair( event, probability ) );

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
        for( auto& entry : *this )
        {
            entry.second = entry.second / total ;
        }
    }

    void Event2ProbabilityMapType::serialize( IArchive& ar, Event2ProbabilityMapType& mapping )
    {
        size_t count = ar.IsWriter() ? mapping.size() : -1;

        ar.startArray(count);
        if( ar.IsWriter() )
        {
            for (auto& entry : mapping)
            {
                std::string key   = entry.first;
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
                event.parameter_name = "HIVRandomChoices::Choices::Event" ;
                event = key;

                mapping[key] = value;
            }
        }
        ar.endArray();
    }

    json::QuickBuilder
    Event2ProbabilityMapType::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:Event2ProbabilityMapType" );

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
    }

    HIVRandomChoice::HIVRandomChoice( const HIVRandomChoice& master )
        : HIVSimpleDiagnostic( master )
    {
        event2ProbabilityMap = master.event2ProbabilityMap;
    }

    bool HIVRandomChoice::Configure( const Configuration* inputJson )
    {
        initConfigComplexType("Choices", &event2ProbabilityMap, HIV_Random_Choices_DESC_TEXT);

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

        std::string eventEnum = NO_TRIGGER_STR ;
        float probSum = 0;

        // pick the IndividualEventTriggerType to broadcast
        for (auto ep : event2ProbabilityMap)
        {
            probSum += ep.second;
            if (p <= probSum)
            {
                eventEnum = ep.first ;
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
        if( eventEnum != NO_TRIGGER_STR )
        {
            broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), eventEnum );
        }
    }

    REGISTER_SERIALIZABLE(HIVRandomChoice);

    void HIVRandomChoice::serialize(IArchive& ar, HIVRandomChoice* obj)
    {
        HIVSimpleDiagnostic::serialize( ar, obj );
        HIVRandomChoice& choice = *obj;

        ar.labelElement("event2ProbabilityMap") & choice.event2ProbabilityMap;
    }
}
