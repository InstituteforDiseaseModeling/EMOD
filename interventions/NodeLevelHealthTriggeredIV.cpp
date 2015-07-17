/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "NodeLevelHealthTriggeredIV.h"

#include "InterventionFactory.h"
#include "Log.h"
#include "Debug.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "SimulationConfig.h"  // for verifying string triggers are 'valid'

static const char * _module = "NodeLevelHealthTriggeredIV";

namespace Kernel
{
    void
    NodeLevelHealthTriggeredIV::tPropertyRestrictions::ConfigureFromJsonAndKey(
        const Configuration * inputJson,
        const std::string& key
    )
    {
        // Make this optional.
        if( inputJson->Exist( key ) == false )
        {
            return;
        }

        // We have a list of json objects. The format/logic is as follows. For input:
        // [
        //  { "Character": "Good", "Income": "High" },
        //  { "Character": "Bad", "Income": "Low" }
        // ]
        // We give the intervention if the individuals has
        // Good Character AND High Income OR Bad Character AND Low Income.
        // So we AND together the elements of each json object and OR together these 
        // calculated truth values of the elements of the json array.
        json::QuickInterpreter s2sarray = (*inputJson)[key].As<json::Array>();
        for( int idx=0; idx < (*inputJson)[key].As<json::Array>().Size(); idx++ )
        {
            std::map< std::string, std::string > kvp;
            auto json_map = s2sarray[idx].As<json::Object>();
            for( auto data = json_map.Begin();
                      data != json_map.End();
                      ++data )
            {
                std::string key = data->name;
                std::string value = (std::string)s2sarray[idx][key].As< json::String >();
                kvp.insert( std::make_pair( key, value ) );
            }
            _restrictions.push_back( kvp );
        }
    }

    json::QuickBuilder
    NodeLevelHealthTriggeredIV::tPropertyRestrictions::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();

        schema[ tn ] = json::String( "idmType:PropertyRestrictions" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        schema[ ts ][0]["<key>"] = json::Object();
        schema[ ts ][0]["<key>"][ "type" ] = json::String( "Constrained String" );
        schema[ ts ][0]["<key>"][ "constraints" ] = json::String( "<demographics>::Defaults.Individual_Properties.*.Property.<keys>" );
        schema[ ts ][0]["<key>"][ "description" ] = json::String( "Individual Property Key from demographics file." );
        schema[ ts ][0]["<value>"] = json::Object();
        schema[ ts ][0]["<value>"][ "type" ] = json::String( "String" );
        schema[ ts ][0]["<key>"][ "constraints" ] = json::String( "<demographics>::Defaults.Individual_Properties.*.Value.<keys>" );
        schema[ ts ][0]["<value>"][ "description" ] = json::String( "Individual Property Value from demographics file." );
        return schema;
    }

    BEGIN_QUERY_INTERFACE_BODY(NodeLevelHealthTriggeredIV)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IIndividualEventObserver)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(NodeLevelHealthTriggeredIV)

    IMPLEMENT_FACTORY_REGISTERED(NodeLevelHealthTriggeredIV)

    NodeLevelHealthTriggeredIV::NodeLevelHealthTriggeredIV()
    : parent(nullptr)
    , duration(0)
    , max_duration(0)
    , demographic_coverage(1)
    , property_restrictions_verified( false )
    , m_disqualified_by_coverage_only(false)
    , target_age_min(0.0f)
    , target_age_max(FLT_MAX)
    , _di(nullptr)
    {
        initConfigComplexType("Actual_IndividualIntervention_Config", &actual_intervention_config, BT_Actual_Intervention_Config_DESC_TEXT);
        initConfigTypeMap("Duration", &max_duration, BT_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f ); // -1 is a convention for indefinite duration
        initConfigTypeMap("Demographic_Coverage", &demographic_coverage, BT_Demographic_Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f );

        //ugh should do the same thing like standard event coordinator but then I have to add targetdemographic to all my files
        initConfigTypeMap( "Target_Age_Min", &target_age_min, Target_Age_Min_DESC_TEXT, 0.0f, FLT_MAX, 0.0f); 
        initConfigTypeMap( "Target_Age_Max", &target_age_max, Target_Age_Max_DESC_TEXT, 0.0f, FLT_MAX, FLT_MAX);

        initConfigComplexType("Property_Restrictions_Within_Node", &property_restrictions, Property_Restriction_DESC_TEXT, "Intervention_Config.*.iv_type", "IndividualTargeted" );
    }

    NodeLevelHealthTriggeredIV::~NodeLevelHealthTriggeredIV() { }
    int NodeLevelHealthTriggeredIV::AddRef()
    {
        return BaseNodeIntervention::AddRef();
    }
    int NodeLevelHealthTriggeredIV::Release()
    {
        return BaseNodeIntervention::Release();
    }

    bool
    NodeLevelHealthTriggeredIV::ConfigureTriggers(
        const Configuration* inputJson
    )
    {
        // First, read in Trigger_Condition as an enum. 3 possible value (types):
        // 1) Actual enum (not NoTrigger). 
        // 2) TriggerList (of strings). Read directly into m_trigger_conditions as string list.
        // 3) TriggerString. Read directly into trigger_condition_string and then push into m_trigger_conditions as single string.
        // Either way, we end up with m_trigger_conditions contains 1 or more strings. That's what we use.
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;
        IndividualEventTriggerType::Enum trigger_condition = IndividualEventTriggerType::NoTrigger;
        ConstrainedString trigger_condition_string = NO_TRIGGER_STR;
        initConfig( "Trigger_Condition", trigger_condition, inputJson, MetadataDescriptor::Enum("Trigger_Condition", HTI_Trigger_Condition_DESC_TEXT, MDD_ENUM_ARGS(IndividualEventTriggerType)) );
        if( trigger_condition == IndividualEventTriggerType::TriggerList || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Trigger_Condition_List", &m_trigger_conditions, NLHTI_Trigger_Condition_List_DESC_TEXT ); // TODO need to add conditionality but not supported in all datatypes yet
        }
        // would have else but schema needs to be able to enter both blocks. 
        if( trigger_condition == IndividualEventTriggerType::TriggerString || JsonConfigurable::_dryrun )
        {
            trigger_condition_string.constraints = "<configuration>:Listed_Events.*";
            trigger_condition_string.constraint_param = &GET_CONFIGURABLE(SimulationConfig)->listed_events;
            initConfigTypeMap( "Trigger_Condition_String", &trigger_condition_string, NLHTI_Trigger_Condition_String_DESC_TEXT  ); // TODO need to add conditionality but not supported in all datatypes yet
        }
        bool retValue = JsonConfigurable::Configure( inputJson );
        if( trigger_condition == IndividualEventTriggerType::TriggerString )
        {
            m_trigger_conditions.push_back( trigger_condition_string );
            LOG_INFO_F( "This NLHTI is listening to %s events.\n", trigger_condition_string.c_str() );
        }
        else if( trigger_condition != IndividualEventTriggerType::TriggerList &&
                 trigger_condition != IndividualEventTriggerType::NoTrigger )
        {
            m_trigger_conditions.push_back( IndividualEventTriggerType::pairs::lookup_key( trigger_condition ) );
            LOG_INFO_F( "This NLHTI is listening to %s events.\n", m_trigger_conditions[0].c_str() );
        }
        return retValue;
    }

    bool
    NodeLevelHealthTriggeredIV::Configure(
        const Configuration * inputJson
    )
    {
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;

        bool retValue = ConfigureTriggers( inputJson );

        //this section copied from standardevent coordinator
        if( retValue )
        {
            InterventionValidator::ValidateIntervention( actual_intervention_config._json );
        }
        JsonConfigurable::_useDefaults = false;
        return retValue;    
    }

    bool
    NodeLevelHealthTriggeredIV::Distribute(
        INodeEventContext *pNodeEventContext,
        IEventCoordinator2 *pEC
    )
    {
        LOG_DEBUG_F("Distributed Nodelevel health-triggered intervention to NODE: %d\n", pNodeEventContext->GetId().data);

        // QI to register ourself as a NodeLevelHealthTriggeredIV observer
        INodeTriggeredInterventionConsumer * pNTIC = NULL;
        if (s_OK != pNodeEventContext->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNodeEventContext", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }
        release_assert( pNTIC );
        for( auto &trigger : m_trigger_conditions )
        {
            pNTIC->RegisterNodeEventObserverByString( (IIndividualEventObserver*)this, trigger );
        }

        // We can QI NEC to get the campaign cost observer (it's just the node!)
        ICampaignCostObserver *iCCO;
        if (parent && s_OK != parent->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&iCCO))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ICampaignCostObserver", "INodeEventContext" );
        }
        assert( iCCO );
        return true;
    }

    //returns false if didn't get the intervention
    bool NodeLevelHealthTriggeredIV::notifyOnEvent(
        IIndividualHumanEventContext *pIndiv,
        const std::string& StateChange
    )
    {
        LOG_DEBUG_F("Individual %d experienced event %s, check to see if they pass the conditions before distributing actual_intervention \n",
                    pIndiv->GetInterventionsContext()->GetParent()->GetSuid().data,
                    StateChange.c_str()
                   );

        assert( parent );
        assert( parent->GetRng() );

        //initialize this flag by individual (not by node)
        m_disqualified_by_coverage_only = false;

        if( qualifiesToGetIntervention( pIndiv ) == false )  
        {
            LOG_DEBUG_F("Individual failed to qualify for intervention, m_disqualified_by_coverage_only is %d \n", m_disqualified_by_coverage_only);
            if (m_disqualified_by_coverage_only == true)
            {
                onDisqualifiedByCoverage( pIndiv );
            }
            return false;
        }
        property_restrictions_verified = true;
        

        // Query for campaign cost observer interface from INodeEventContext *parent
        ICampaignCostObserver *iCCO;
        if (s_OK != parent->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&iCCO))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ICampaignCostObserver", "INodeEventContext" );
        }

        // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
        //const IInterventionFactory* ifobj = dynamic_cast<NodeEventContextHost *>(parent)->GetInterventionFactoryObj();
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

        if( _di == nullptr )
        {
            auto config = Configuration::CopyFromElement( (actual_intervention_config._json) );
            _di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention( config ); 
            release_assert( _di );
            delete config;
        }
        // Huge performance win by cloning instead of configuring.
        release_assert( _di );
        IDistributableIntervention *di = _di->Clone();
        release_assert( di );
        di->AddRef();

        auto ret = true;
        {
            if( di->Distribute( pIndiv->GetInterventionsContext(), iCCO ) )
            {
/*
                auto classname = (std::string) json::QuickInterpreter(actual_intervention_config._json)["class"].As<json::String>();
                LOG_DEBUG_F("A Node level health-triggered intervention (%s) was successfully distributed to individual %d\n",
                            classname.c_str(),
                            pIndiv->GetInterventionsContext()->GetParent()->GetSuid().data
                           );
*/
                // It's not at all clear to me that we would incur cost at this point, but we could.
                //iCCO->notifyCampaignExpenseIncurred( interventionCost, pIndiv );
                ret = true;
            }
            di->Release();
        }
        return ret;
    }

    void NodeLevelHealthTriggeredIV::Update( float dt )
    {
        duration += dt;
        if( max_duration >= 0 && duration > max_duration )
        {
            // QI to register ourself as a node level health triggered observer
            LOG_DEBUG_F( "Node-Level HTI reached max_duration. Time to unregister...\n" );
            INodeTriggeredInterventionConsumer * pNTIC = nullptr;
            if (s_OK == parent->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC) )
            {
                release_assert( pNTIC );
                for( auto &trigger : m_trigger_conditions )
                {
                    pNTIC->UnregisterNodeEventObserverByString( this, trigger );
                }
            }
            else
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
        }
    }

    void NodeLevelHealthTriggeredIV::SetContextTo(INodeEventContext *context) 
    { 
        release_assert( context );
        parent = context; 
    }

    // private/protected
    bool
    NodeLevelHealthTriggeredIV::qualifiesToGetIntervention(
        const IIndividualHumanEventContext * const pIndividual
    )
    {
        bool retQualifies = true;

        //based on StandardEventCoordinator pattern, first check if the individual has the right property, then check their age, then check if they are part of the demographic coverage
        
        //this section directly copied from StandardEventCoordinator
        if( property_restrictions._restrictions.size() )
        {
            retQualifies = false;
            // individual has to have one of these properties
            for (auto& prop_map : property_restrictions._restrictions)
            {
                bool meets_property_restriction_criteria = true;
                for( auto& prop : prop_map )
                {
                    const std::string& szKey = prop.first;
                    const std::string& szVal = prop.second;

                    //Verify that the user entered in set of property key/value pairs which are included in the demographics file
                    if( property_restrictions_verified == false )
                    {
                        parent->GetNodeContext()->VerifyPropertyDefined( szKey, szVal );
                        //property_restrictions_verified = true;
                        LOG_DEBUG( "Finished verifying properties \n" );
                    }

                    auto * pProp = const_cast<Kernel::IIndividualHumanEventContext*>(pIndividual)->GetProperties(); // const_cast needed by MSVC, not GCC
                    release_assert( pProp );

                    LOG_DEBUG_F( "Applying property restrictions in NodeLevelHealthTriggeredIV: %s/%s.\n", szKey.c_str(), szVal.c_str() );
                    // Every individual has to have a property value for each property key
                    if( pProp->find( szKey ) == pProp->end() )
                    {
                        throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", szKey.c_str() );
                    }
                    else if( pProp->at( szKey ) == szVal )
                    {
                        LOG_DEBUG_F( "Person satisfies (partial) property restriction: constraint is %s/%s and the person is %s.\n", szKey.c_str(), szVal.c_str(), pProp->at( szKey ).c_str() );
                        continue; // we're good
                    }
                    else
                    {
                        meets_property_restriction_criteria = false;
                        LOG_DEBUG_F( "Person does not get the intervention because the allowed property is %s/%s and the person is %s.\n", szKey.c_str(), szVal.c_str(), pProp->at( szKey ).c_str() );
                        break;
                    }
                }
                // If verified, we're done since these are OR-ed together
                if( meets_property_restriction_criteria )
                {
                    retQualifies = true;
                    LOG_DEBUG_F( "Individual meets at least 1 of the OR-ed together property restriction conditions. Not checking the rest.\n" );
                    break;
                }
            }
        }
        else
        {
            LOG_DEBUG( "No property restrictions in NodeLevelHealthTriggeredIV to apply.\n" );
        }
        
        //OK they passed the property test, now check if they pass the age test
        if (retQualifies)
        {
            if( pIndividual->GetAge() < target_age_min * DAYSPERYEAR )
            {
                LOG_DEBUG_F("Individual not given intervention because too young (age=%f) for intervention min age (%f)\n", pIndividual->GetAge(), target_age_min* DAYSPERYEAR);
                retQualifies = false;
            }
            else if( pIndividual->GetAge() > target_age_max * DAYSPERYEAR )
            {
                LOG_DEBUG_F("Individual not given intervention because too old (age=%f) for intervention max age (%f)\n", pIndividual->GetAge(), target_age_max* DAYSPERYEAR);
                retQualifies = false;
            }

        }

        //OK they passed the property and age test, now check if they are part of the demographic coverage
        if (retQualifies)
        {
            double randomDraw = parent->GetRng()->e();
            LOG_DEBUG_F("demographic_coverage = %f\n", getDemographicCoverage());
            if( randomDraw > getDemographicCoverage() )
            {
                m_disqualified_by_coverage_only = true;
                LOG_DEBUG_F("Demographic coverage ruled this out, m_disqualified_by_coverage_only is %d \n", m_disqualified_by_coverage_only);
                retQualifies = false;
            }
        }

        LOG_DEBUG_F( "Returning %d from %s\n", retQualifies, __FUNCTION__ );
        return retQualifies;
    }
  
    float NodeLevelHealthTriggeredIV::getDemographicCoverage() const
    {
        return demographic_coverage;    
    }

    void NodeLevelHealthTriggeredIV::onDisqualifiedByCoverage( IIndividualHumanEventContext *pIndiv )
    {
    //do nothing, this is for the scale up switch
    }
#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    void NodeLevelHealthTriggeredIV::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__);
    }

    void NodeLevelHealthTriggeredIV::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__);
    }
#endif
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::NodeLevelHealthTriggeredIV)

namespace Kernel {
    REGISTER_SERIALIZATION_VOID_CAST(NodeLevelHealthTriggeredIV, INodeDistributableIntervention)
    REGISTER_SERIALIZATION_VOID_CAST(NodeLevelHealthTriggeredIV, IIndividualEventObserver)
    template<class Archive>
    void serialize(Archive &ar, NodeLevelHealthTriggeredIV& iv, const unsigned int v)
    {
        ar & iv.actual_intervention_config;
        ar & iv.demographic_coverage;
        ar & iv.efficacy;
        ar & iv.max_duration;
        ar & iv.m_trigger_conditions;
    }
}
#endif
