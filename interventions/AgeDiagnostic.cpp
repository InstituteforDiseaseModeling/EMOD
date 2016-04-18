/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "AgeDiagnostic.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanHIV.h"
#include "SusceptibilityHIV.h"

static const char * _module = "AgeDiagnostic";

namespace Kernel
{
    void
    AgeThresholds::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        LOG_DEBUG_F( "Configuring age thresholds from campaign.json\n" );
        // Now's as good a time as any to parse in the calendar schedule.
        json::QuickInterpreter a_qi( (*inputJson)[key] );
        try {
            json::QuickInterpreter threshJson( a_qi.As<json::Array>() );
            assert( a_qi.As<json::Array>().Size() );
            for( unsigned int idx=0; idx<a_qi.As<json::Array>().Size(); idx++ )
            {
                EventTrigger signal;
                initConfigTypeMap( "Event", &signal, HIV_Age_Diagnostic_Event_Name_DESC_TEXT );
                auto obj = Configuration::CopyFromElement((threshJson)[idx]);
                JsonConfigurable::Configure( obj );
                delete obj;
                obj = nullptr;
                thresh_events.push_back( signal );

                NaturalNumber low, high;
                try {
                    low = float(threshJson[idx]["Low"].As<json::Number>());
                }
                catch( const json::Exception & )
                {
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Low", threshJson[idx], "Expected NUMBER" );
                }
                try {
                    high = float(threshJson[idx]["High"].As<json::Number>());
                }
                catch( const json::Exception & )
                {
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, "High", threshJson[idx], "Expected NUMBER" );
                }
                if( high <= low )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                            "low",  std::to_string( low ).c_str(),
                            "high", std::to_string( high ).c_str(),
                            "High value must be higher than Low value." );
                }

                thresholds.push_back( std::make_pair( low, high ) );
                LOG_DEBUG_F( "Found age threshold set from config: low/high/event = %d/%d/%s\n", (int) low, (int) high, signal.c_str() );
            }
        }
        catch( const json::Exception & )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), a_qi, "Expected ARRAY" );
        }
        LOG_DEBUG_F( "Found %d age thresholds\n", thresholds.size() );
    }

    static void serialize_thresholds( IArchive& ar, std::vector<std::pair<NaturalNumber,NaturalNumber>>& thresholds )
    {
        size_t count = ar.IsWriter() ? thresholds.size() : -1;

        ar.startArray(count);
        if( !ar.IsWriter() ) 
        {
            thresholds.resize(count);
        }
        for( auto& entry : thresholds )
        {
            ar.startObject();
            ar.labelElement("first" ) & entry.first;
            ar.labelElement("second") & entry.second;
            ar.endObject();
        }
        ar.endArray();
    }

    void AgeThresholds::serialize(IArchive& ar, AgeThresholds& obj)
    {
        ar.startObject();
        ar.labelElement("thresholds"   ); serialize_thresholds( ar, obj.thresholds );
        ar.labelElement("thresh_events") & obj.thresh_events;
        ar.endObject();

        // verify events are valid
        if( ar.IsReader() )
        {
            EventTrigger signal;
            for( auto ev : obj.thresh_events )
            {
                signal = ev;
            }
        }
    }

    json::QuickBuilder
    AgeThresholds::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:AgeThresholds" );
        schema[ts] = json::Array();
        schema[ts][0] = json::Object();
        schema[ts][0]["Low"] = json::Object();
        schema[ts][0]["Low"][ "type" ] = json::String( "float" );
        schema[ts][0]["Low"][ "min" ] = json::Number( 0 );
        schema[ts][0]["Low"][ "max" ] = json::Number( 1000.0 );
        schema[ts][0]["Low"][ "description" ] = json::String( HIV_Age_Diagnostic_Low_DESC_TEXT );
        schema[ts][0]["High"] = json::Object();
        schema[ts][0]["High"][ "type" ] = json::String( "float" );
        schema[ts][0]["High"][ "min" ] = json::Number( 0 );
        schema[ts][0]["High"][ "max" ] = json::Number( 1000.0 );
        schema[ts][0]["High"][ "description" ] = json::String( HIV_Age_Diagnostic_High_DESC_TEXT );
        schema[ts][0]["Event"] = json::Object();
        schema[ts][0]["Event"][ "type" ] = json::String( "String" );
        schema[ts][0]["Event"][ "description" ] = json::String( HIV_Age_Diagnostic_Event_Name_DESC_TEXT );
        return schema;
    }

    BEGIN_QUERY_INTERFACE_DERIVED(AgeDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(AgeDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(AgeDiagnostic)

    bool AgeDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("Age_Thresholds", &age_thresholds, HIV_Age_Thresholds_DESC_TEXT );
        return JsonConfigurable::Configure(inputJson);
    }

    AgeDiagnostic::AgeDiagnostic() : SimpleDiagnostic()
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    AgeDiagnostic::AgeDiagnostic( const AgeDiagnostic& master )
    : SimpleDiagnostic( master )
    {
        age_thresholds = master.age_thresholds;
    }

    AgeDiagnostic::~AgeDiagnostic()
    {
        LOG_DEBUG("Destructing Age Diagnostic \n");
    }

    bool AgeDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test result function\n");

        // Apply diagnostic test with given specificity/sensitivity
        bool test_pos = false;

        IIndividualHumanEventContext* ind_hec = nullptr;
        if(parent->QueryInterface( GET_IID( IIndividualHumanEventContext ), (void**)&ind_hec ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanEventContext", "IIndividualHuman" );
        }

        float age_years = ind_hec->GetAge() / DAYSPERYEAR;
        LOG_DEBUG_F( "age is %f. %d thresholds configured.\n", age_years, age_thresholds.thresholds.size() );

        unsigned int thresh_event_counter = 0;
        for( auto thresh : age_thresholds.thresholds )
        {
            LOG_DEBUG_F( "low/high thresholds = %d/%d\n", (int) thresh.first, (int) thresh.second );
            if( age_years >= thresh.first && age_years < thresh.second )
            {
                // broadcast associated event
                INodeTriggeredInterventionConsumer* broadcaster = nullptr;
                if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
                }
                LOG_DEBUG_F("SimpleHealthSeekingBehavior is broadcasting the actual intervention event to individual %d.\n", parent->GetSuid().data );
                broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), age_thresholds.thresh_events[ thresh_event_counter ] );
                test_pos = true;
            }
            thresh_event_counter++;
        }

        // always return negative if the person is not infected, intended to be used with GroupEventCoordinator
        // TODO: allow to distribute Smear diagnostic to non-infected individuals?

        // True positive (sensitivity), or False positive (1-specificity)
        expired = true;
        bool positiveTest = applySensitivityAndSpecificity( test_pos );
        return positiveTest;
    }

    REGISTER_SERIALIZABLE(AgeDiagnostic);

    void AgeDiagnostic::serialize(IArchive& ar, AgeDiagnostic* obj)
    {
        SimpleDiagnostic::serialize( ar, obj );
        AgeDiagnostic& ad = *obj;
        ar.labelElement("age_thresholds"); AgeThresholds::serialize( ar, ad.age_thresholds );
    }
}
