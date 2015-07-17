/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "CD4Diagnostic.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanHIV.h"
#include "SusceptibilityHIV.h"
#include "SimulationConfig.h"

static const char * _module = "CD4Diagnostic";

namespace Kernel
{
    void
    CD4Thresholds::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        LOG_DEBUG_F( "Configuring CD4 thresholds from campaign.json\n" );
        // Now's as good a time as any to parse in the calendar schedule.
        json::QuickInterpreter a_qi( (*inputJson)[key] );
        json::QuickInterpreter threshJson( a_qi.As<json::Array>() );
        assert( a_qi.As<json::Array>().Size() );
        for( unsigned int idx=0; idx<a_qi.As<json::Array>().Size(); idx++ )
        {
            ConstrainedString signal = "UNINITIALIZED";
            signal.constraints = "<configuration>:Listed_Events.*";
            signal.constraint_param = &GET_CONFIGURABLE(SimulationConfig)->listed_events;
            initConfigTypeMap( "Event", &signal, HIV_CD4_Diagnostic_Event_Name_DESC_TEXT );
            auto obj = Configuration::CopyFromElement((threshJson)[idx]);
            JsonConfigurable::Configure( obj );
            delete obj;
            thresh_events.push_back( signal );

            NaturalNumber low = float(threshJson[idx]["Low"].As<json::Number>());
            NaturalNumber high = float(threshJson[idx]["High"].As<json::Number>());
            if( high <= low )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "low", std::to_string( low ).c_str(),
                                                                                          "high", std::to_string( high ).c_str(), 
                                                                                          "High value must be higher than Low value." );
            }

            thresholds.push_back( std::make_pair( low, high ) );
            LOG_DEBUG_F( "Found CD4 threshold set from config: low/high/event = %d/%d/%s\n", (int) low, (int) high, signal.c_str() );
        }
        LOG_DEBUG_F( "Found %d CD4 thresholds\n", thresholds.size() );
    }

    json::QuickBuilder
    CD4Thresholds::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:CD4Thresholds" );
        schema[ts] = json::Array();
        schema[ts][0] = json::Object();
        schema[ts][0]["Low"] = json::Object();
        schema[ts][0]["Low"][ "type" ] = json::String( "float" );
        schema[ts][0]["Low"][ "min" ] = json::Number( 0 );
        schema[ts][0]["Low"][ "max" ] = json::Number( 1000.0 );
        schema[ts][0]["Low"][ "description" ] = json::String( HIV_CD4_Diagnostic_High_DESC_TEXT );
        schema[ts][0]["High"] = json::Object();
        schema[ts][0]["High"][ "type" ] = json::String( "float" );
        schema[ts][0]["High"][ "min" ] = json::Number( 0 );
        schema[ts][0]["High"][ "max" ] = json::Number( 1000.0 );
        schema[ts][0]["High"][ "description" ] = json::String( HIV_CD4_Diagnostic_Low_DESC_TEXT );
        schema[ts][0]["Event"] = json::Object();
        schema[ts][0]["Event"][ "type" ] = json::String( "String" );
        schema[ts][0]["Event"][ "description" ] = json::String( HIV_CD4_Diagnostic_Event_Name_DESC_TEXT );
        return schema;
    }

    BEGIN_QUERY_INTERFACE_DERIVED(CD4Diagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(CD4Diagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(CD4Diagnostic)

    bool CD4Diagnostic::Configure(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("CD4_Thresholds", &cd4_thresholds, HIV_CD4_Thresholds_DESC_TEXT);
        return JsonConfigurable::Configure(inputJson);
    }

    CD4Diagnostic::CD4Diagnostic() : SimpleDiagnostic()
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    CD4Diagnostic::CD4Diagnostic( const CD4Diagnostic& master )
    : SimpleDiagnostic( master )
    {
        cd4_thresholds = master.cd4_thresholds;
    }
        
    CD4Diagnostic::~CD4Diagnostic()
    { 
        LOG_DEBUG("Destructing CD4 Diagnostic \n");
    }

    bool CD4Diagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test Result function\n");

        // Apply diagnostic test with given specificity/sensitivity
        bool test_pos = false;
        float rand = parent->GetRng()->e();

        IIndividualHumanHIV* hiv_ind = NULL;
        if(parent->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&hiv_ind ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHuman" );
        }

        auto cd4count = hiv_ind->GetHIVSusceptibility()->GetCD4count();
        LOG_DEBUG_F( "cd4count measured at %f. %d thresholds configured.\n", cd4count, cd4_thresholds.thresholds.size() );

        unsigned int thresh_event_counter = 0;
        for( auto thresh : cd4_thresholds.thresholds )
        {
            LOG_DEBUG_F( "low/high thresholds = %d/%d\n", (int) thresh.first, (int) thresh.second );
            if( cd4count >= thresh.first && cd4count < thresh.second )
            {
                // broadcast associated event
                INodeTriggeredInterventionConsumer* broadcaster = nullptr;
                if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
                }
                LOG_DEBUG_F("SimpleHealthSeekingBehavior is broadcasting the actual intervention event to individual %d.\n", parent->GetSuid().data );
                broadcaster->TriggerNodeEventObserversByString( parent->GetEventContext(), cd4_thresholds.thresh_events[ thresh_event_counter ] );
                test_pos = true;
            }
            thresh_event_counter++;
        }

        // always return negative if the person is not infected, intended to be used with GroupEventCoordinator
        // TODO: allow to distribute Smear diagnostic to non-infected individuals?

        bool positiveTest = false;
        // True positive (sensitivity), or False positive (1-specificity)
        positiveTest = ( test_pos && (rand < base_sensitivity) ) || ( !test_pos && (rand > base_specificity) );
        expired = true;
        return positiveTest;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::CD4Diagnostic)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, CD4Diagnostic& obj, const unsigned int v)
    {

        boost::serialization::void_cast_register<CD4Diagnostic, IDistributableIntervention>();

        ar & boost::serialization::base_object<Kernel::SimpleDiagnostic>(obj);
    }
}
#endif
