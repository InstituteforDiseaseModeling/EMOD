/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "CalendarIV.h"

#include <stdlib.h>

#include "Debug.h"
#include "CajunIncludes.h"     // for parsing calendar and actual interventions
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)

static const char * _module = "IVCalendar";

namespace Kernel
{
    void
    TargetAgeArrayConfig::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        // Now's as good a time as any to parse in the calendar schedule.
        json::QuickInterpreter taa_qi( (*inputJson)[key] );
        json::QuickInterpreter scheduleJson( taa_qi.As<json::Array>() );
        assert( taa_qi.As<json::Array>().Size() );
        for( unsigned int idx=0; idx<taa_qi.As<json::Array>().Size(); idx++ )
        {
            float age = float(scheduleJson[idx]["Age"].As<json::Number>());
            float probability = float(scheduleJson[idx]["Probability"].As<json::Number>());
            age2ProbabilityMap.insert( std::make_pair( age, probability ) );
        }
    }

    json::QuickBuilder
    TargetAgeArrayConfig::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:CalendarIV" );
    
        schema[ts] = json::Array();
        schema[ts][0] = json::Object();
        schema[ts][0]["Age"] = json::Object();
        schema[ts][0]["Age"][ "type" ] = json::String( "float" );
        schema[ts][0]["Age"][ "min" ] = json::Number( 0 );
        schema[ts][0]["Age"][ "max" ] = json::Number( MAX_HUMAN_AGE*DAYSPERYEAR );
        schema[ts][0]["Age"][ "description" ] = json::String( CAL_Age_DESC_TEXT );
        schema[ts][0]["Probability"] = json::Object();
        schema[ts][0]["Probability"][ "type" ] = json::String( "float" );
        schema[ts][0]["Probability"][ "min" ] = json::Number( 0 );
        schema[ts][0]["Probability"][ "max" ] = json::Number( 1.0 );
        schema[ts][0]["Probability"][ "description" ] = json::String( CAL_Probability_DESC_TEXT );
        return schema;
    }

    BEGIN_QUERY_INTERFACE_BODY(IVCalendar)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IBaseIntervention)
    END_QUERY_INTERFACE_BODY(IVCalendar)

    IMPLEMENT_FACTORY_REGISTERED(IVCalendar)

    IVCalendar::IVCalendar()
    : parent(NULL)
    , dropout(false)
    {
        initConfigTypeMap("Dropout", &dropout, CAL_Dropout_DESC_TEXT, false);
    }

    IVCalendar::~IVCalendar()
    {
        LOG_DEBUG("Calendar was destroyed.\n");
    }

    bool
    IVCalendar::Configure(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("Calendar", &target_age_array, CAL_Calendar_DESC_TEXT);
        initConfigComplexType("Actual_IndividualIntervention_Configs", &actual_intervention_config, CAL_Actual_Intervention_Configs_DESC_TEXT);

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret )
        {
            InterventionValidator::ValidateInterventionArray( actual_intervention_config._json );
        }
        return ret ;
    }

    bool
    IVCalendar::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        parent = context->GetParent(); // is there a better way to get the parent?

        LOG_DEBUG("IVCalendar::Distribute\n");
        release_assert( parent );
        release_assert( parent->GetRng() );

        // Now's as good a time as any to parse in the calendar schedule.
        for( auto &entry: target_age_array.age2ProbabilityMap )
        {
            float age = entry.first;
            float probability = entry.second;

            double randomDraw = parent->GetRng()->e();
            if( randomDraw < probability )
            {
                scheduleAges.push_back( age );
            }
            else if( dropout )
            {
                LOG_DEBUG_F("dropout = true, so since %f dose was missed, all others missed as well\n", age);
                break;
            }
            else
            {
                LOG_DEBUG_F("Calendar stochastically rejected vaccine dose at age %f, but dropout = false so still might get others\n", age);
            }
        }

        LOG_DEBUG_F("%s\n", dumpCalendar().c_str());

        // Purge calendar entries that are in the past for this individual
        release_assert( parent->GetEventContext() );
        while( scheduleAges.size() > 0 && parent->GetEventContext()->GetAge() > scheduleAges.front() )
        {
            LOG_DEBUG("Calender given to individual already past age for part of schedule. Purging.\n" );
            scheduleAges.pop_front();
            if( scheduleAges.size() == 0 )
            {
                expired = true;
            }
        }

        return BaseIntervention::Distribute( context, pICCO );
    }

    // Each time this is called, the HSB intervention is going to decide for itself if
    // health should be sought. For start, just do it based on roll of the dice. If yes,
    // an intervention needs to be created (from what config?) and distributed to the 
    // individual who owns us. Hmm...
    void IVCalendar::Update( float dt )
    {
        //calendar_age += dt;
        if( ( scheduleAges.size() > 0 ) && ( parent->GetEventContext()->GetAge() >= scheduleAges.front() ) )
        {
            scheduleAges.pop_front();
            if( scheduleAges.size() == 0 )
            {
                expired = true;
            }
            LOG_DEBUG_F("Calendar says it's time to apply an intervention...\n");
            // Check if actual_intervention_config is an array instead of JObject
            try
            {
                const json::Array & interventions_array = json::QuickInterpreter( actual_intervention_config._json ).As<json::Array>();
                LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());

                // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
                IGlobalContext *pGC = NULL;
                const IInterventionFactory* ifobj = NULL;
                if (s_OK == parent->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
                {
                    ifobj = pGC->GetInterventionFactory();
                }
                if (!ifobj)
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
                }
                for( int idx=0; idx<interventions_array.Size(); idx++ )
                {
                    const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[idx]);
                    Configuration * tmpConfig = Configuration::CopyFromElement(actualIntervention);
                    assert( tmpConfig );
                    IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention(tmpConfig);
                    delete tmpConfig;
                    if( !di )
                    {
                        // Calendar wanted to distribute intervention but factory returned null pointer.
                        throw FactoryCreateFromJsonException( __FILE__, __LINE__, __FUNCTION__, "Unable to create intervention object from actualIntervention (apparently). Factory should actually throw exception. This exception throw is just paranoid exception handling." );
                    }
                    float interventionCost = 0.0f; 
                    LOG_DEBUG_F("Calendar (intervention) distributed actual intervention at age %f\n", parent->GetEventContext()->GetAge());

                    // Now make sure cost gets reported.
                    ICampaignCostObserver* pICCO;
                    assert( parent->GetEventContext()->GetNodeEventContext() );
                    if (s_OK == parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&pICCO) )
                    {
                        di->Distribute( parent->GetInterventionsContext(), pICCO );
                    }
                    else
                    {
                        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext" );
                    }
                }
            }
            catch(json::Exception &e)
            {
                // ERROR: ::cerr << "exception casting actual_intervention_config to array! " << e.what() << std::endl;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); // ( "Calendar intervention json problem: actual_intervention_config is valid json but needs to be an array." );
            }
        }
        // TODO: Calendar may be done, should be disposed of somehow. How about parent->Release()??? :)
    }

    // This is a debug only utility function to dump out actual dosing calendars.
    std::string IVCalendar::dumpCalendar()
    {
        std::ostringstream msg;
        msg << "Dose Calendar: ";
        for (float age : scheduleAges)
        {
            msg << age << ",";
        }
        return msg.str();
    }

}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::IVCalendar)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, IVCalendar& cal, const unsigned int v)
    {
        boost::serialization::void_cast_register<IVCalendar, IDistributableIntervention>();
        ar & cal.actual_intervention_config;
        ar & cal.scheduleAges;
        ar & cal.target_age_array;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(cal);
    }
}
#endif
