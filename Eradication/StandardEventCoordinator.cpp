/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include <sstream>
#include "Debug.h"
#include "Sugar.h"
#include "Environment.h"
#include "StandardEventCoordinator.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "FactorySupport.h"
#include "InterventionFactory.h"
#include "Individual.h"
#include "Node.h"
#include "NodeEventContext.h"
#include "Log.h"
#include "IdmString.h"

static const char * _module = "StandardEventCoordinator";


// Let's have some fun and customize this. Log out which nodes get the intervention, and which inviduals, track
// them in a map, make bednet's conditional on some node info we get, and some individual info (not just coverage/
// randomness, then lets' repeat in 30 days 1 time.
// Then test out taking bednets away from migrators (just as a test).
namespace Kernel
{

    IMPLEMENT_FACTORY_REGISTERED(StandardInterventionDistributionEventCoordinator)

    IMPL_QUERY_INTERFACE2(StandardInterventionDistributionEventCoordinator, IEventCoordinator, IConfigurable)

    QuickBuilder
    StandardInterventionDistributionEventCoordinator::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    // ctor
    StandardInterventionDistributionEventCoordinator::StandardInterventionDistributionEventCoordinator()
    : parent(NULL)
    , coverage(0)
    , distribution_complete(false)
    , num_repetitions(-1)
    , tsteps_between_reps(-1)
    , intervention_activated(false)
    , tsteps_since_last(0)
    , target_age_min(0)
    , target_age_max(0)
    , target_gender(TargetGender::All)
    , demographic_coverage(0)
    , travel_linked(false)
    , include_emigrants(0)
    , include_immigrants(0)
    , property_restrictions_verified( false )
    , _di( nullptr )
    {
        LOG_DEBUG("StandardInterventionDistributionEventCoordinator ctor\n");
    }

    bool
    StandardInterventionDistributionEventCoordinator::Configure(
        const Configuration * inputJson
    )
    {
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;

        initializeInterventionConfig( inputJson );

        //initConfigTypeMap("Number_Distributions", &num_distributions, Number_Distributions_DESC_TEXT, -1, 1e6, -1 ); // by convention, -1 means no limit
        initConfigTypeMap("Demographic_Coverage", &demographic_coverage, Demographic_Coverage_DESC_TEXT, 0.0, 1.0, 1.0, "Intervention_Config.*.iv_type", "IndividualTargeted" );

        initConfigTypeMap("Number_Repetitions", &num_repetitions, Number_Repetitions_DESC_TEXT, -1, 1000, -1 );
        //if( num_repetitions > 1 ) // -1 = repeat without end, 0 is meaningless. want to think this one through more
        {
            initConfigTypeMap("Timesteps_Between_Repetitions", &tsteps_between_reps, Timesteps_Between_Repetitions_DESC_TEXT, -1, 10000 /*undefined*/, -1 /*off*/, "Number_Repetitions", "<>0" );
        }
        initConfigTypeMap("Travel_Linked", &travel_linked, Travel_Linked_DESC_TEXT, 0, 1, 0, "Intervention_Config.*.iv_type", "IndividualTargeted" );
        initConfigTypeMap("Include_Departures", &include_emigrants, Include_Departures_DESC_TEXT, 0, 1, 0, "Travel_Linked", "true" );
        initConfigTypeMap("Include_Arrivals", &include_immigrants, Include_Arrivals_DESC_TEXT, 0, 1, 0, "Travel_Linked", "true" );

        initConfig( "Target_Demographic", target_demographic, inputJson, MetadataDescriptor::Enum("target_demographic", Target_Demographic_DESC_TEXT, MDD_ENUM_ARGS(TargetDemographicType)), "Intervention_Config.*.iv_type", "IndividualTargeted");
        if ( target_demographic == TargetDemographicType::ExplicitAgeRanges
            || target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender
            || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Target_Age_Min", &target_age_min, Target_Age_Min_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Target_Demographic", "ExplicitAgeRanges" );
            initConfigTypeMap( "Target_Age_Max", &target_age_max, Target_Age_Max_DESC_TEXT, 0.0f, FLT_MAX, FLT_MAX, "Target_Demographic", "ExplicitAgeRanges" );
            if( target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender || JsonConfigurable::_dryrun)
            {
                initConfig( "Target_Gender", target_gender, inputJson, MetadataDescriptor::Enum("target_gender", Target_Gender_DESC_TEXT, MDD_ENUM_ARGS(TargetGender)) ); 
            }
        } else if ( target_demographic == TargetDemographicType::ExplicitGender || JsonConfigurable::_dryrun )
        {
            initConfig( "Target_Gender", target_gender, inputJson, MetadataDescriptor::Enum("target_gender", Target_Gender_DESC_TEXT, MDD_ENUM_ARGS(TargetGender)) ); 
        }

        property_restrictions.value_source = "<demographics>::Defaults.Individual_Properties.*.Property.<keys>:<demographics>::Defaults.Individual_Properties.*.Value.<keys>"; 
        // xpath-y way of saying that the possible values for prop restrictions comes from demographics file IP's.
        initConfigTypeMap("Property_Restrictions", &property_restrictions, Property_Restriction_DESC_TEXT, "Intervention_Config.*.iv_type", "IndividualTargeted" );

        bool retValue = JsonConfigurable::Configure( inputJson );
        // check if property_restrictions are in the list
        std::set< std::string > dupKeyHelperSet;
        for (const auto& prop : property_restrictions)
        {
            // parse, pre-colon is prop key, post is value
            size_t sep = prop.find( ':' );
            const std::string& szKey = prop.substr( 0, sep );
            const std::string& szVal = prop.substr( sep+1, prop.size()-sep );
            if( dupKeyHelperSet.count( szKey ) != 0 )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Duplicate keys in property_restrictions. Since entries are AND-ed together, this will always be an empty set since an individual can only have a single value of a given key. Use second intervention instead." );
            }
            dupKeyHelperSet.insert( szKey );
            property_restrictions_map.insert( make_pair( szKey, szVal ) );
        }

        if( retValue )
        {
            validateInterventionConfig( intervention_config._json );
        }

        JsonConfigurable::_useDefaults = false;
        return retValue;
    }

    void StandardInterventionDistributionEventCoordinator::initializeInterventionConfig(
        const Configuration* inputJson
    )
    {
        initConfigComplexType("Intervention_Config", &intervention_config, Intervention_Config_DESC_TEXT );
    }

    void StandardInterventionDistributionEventCoordinator::validateInterventionConfig( const json::Element& rElement )
    {
        InterventionValidator::ValidateIntervention( rElement );
    }

    void
    StandardInterventionDistributionEventCoordinator::SetContextTo(
        ISimulationEventContext *isec
    )
    {
        parent = isec;
        regenerateCachedNodeContextPointers();
    }

    // AddNode
    // EventCoordinators track nodes. Nodes can be used to get individuals, who can be queried for an intervention
    void
    StandardInterventionDistributionEventCoordinator::AddNode(
        const suids::suid& node_suid
    )
    {
        if( !intervention_activated )
        {
            intervention_activated = true;
            tsteps_since_last = tsteps_between_reps -1; // -1 is hack because Update is called before UpdateNodes and we inc in Update and check in UpdateNodes
        }

        // Store uids and node (event context) pointers
        node_suids.push_back(node_suid);
        cached_nodes.push_back(parent->GetNodeEventContext(node_suid));

        INodeEventContext * pNec = parent->GetNodeEventContext(node_suid);
        // Register unconditionally to be notified when individuals arrive at our node so we can zap them!
        // TODO: Make this param driven
        if( include_immigrants )
        {
            pNec->RegisterTravelDistributionSource( this, INodeEventContext::Arrival );
        }
        if( include_emigrants )
        {
            pNec->RegisterTravelDistributionSource( this, INodeEventContext::Departure );
        }
    }

    void StandardInterventionDistributionEventCoordinator::Update( float dt )
    {
        // Check if it's time for another distribution
        if( intervention_activated && num_repetitions)
        {
            tsteps_since_last++;
        }
    }

    /*
    float StandardInterventionDistributionEventCoordinator::getDemographicCoverage
    const
    ()
    {
        return demographic_coverage;
    }
    */
    void StandardInterventionDistributionEventCoordinator::UpdateNodes( float dt )
    {
        // Only call VisitNodes on first call and if countdown == 0
        if( tsteps_since_last != tsteps_between_reps )
        {
            return;
        }

        int grandTotal = 0;
        int limitPerNode = -1;

        // intervention class names for informative logging
        std::ostringstream intervention_name;
        intervention_name << std::string( json::QuickInterpreter(intervention_config._json)["class"].As<json::String>() );

        auto qi_as_config = Configuration::CopyFromElement( (intervention_config._json) );
        _di = InterventionFactory::getInstance()->CreateIntervention(qi_as_config);

        // including deeper information for "distributing" interventions (e.g. calendars)
        formatInterventionClassNames( intervention_name, &json::QuickInterpreter(intervention_config._json) );

        // Only visit individuals if this is NOT an NTI. Check...
        // Check to see if intervention is an INodeDistributable...
        INodeDistributableIntervention *ndi = InterventionFactory::getInstance()->CreateNDIIntervention(qi_as_config);
        INodeDistributableIntervention *ndi2 = NULL;

        //LOG_DEBUG_F("[UpdateNodes] limitPerNode = %d\n", limitPerNode);
        LOG_DEBUG_F("[UpdateNodes] visiting %d nodes per NodeSet\n", cached_nodes.size());
        for (auto event_context : cached_nodes)
        {
            if( ndi )
            {
                ndi2 = InterventionFactory::getInstance()->CreateNDIIntervention(qi_as_config);
                if(ndi2)
                {
                    if( ndi2->Distribute( event_context, this ) )
                    {
                        LOG_INFO_F("UpdateNodes() distributed '%s' intervention to node %d\n", intervention_name.str().c_str(), event_context->GetId().data );
                    }
                    ndi2->Release();
                }
            }
            else
            {
                // For now, distribute evenly across nodes. 
                int totalIndivGivenIntervention = event_context->VisitIndividuals( this, limitPerNode );
                grandTotal += totalIndivGivenIntervention;

                // Create log message 
                std::stringstream ss;
                ss << "UpdateNodes() gave out " << totalIndivGivenIntervention << " '" << intervention_name.str().c_str() << "' interventions ";
                if( property_restrictions.size() > 0 )
                {
                    std::string restriction_str ;
                    for( auto prop : property_restrictions )
                    {
                        restriction_str += "'"+ prop +"', " ;
                    }
                    restriction_str = restriction_str.substr( 0, restriction_str.length()-2 );
                    ss << "with property restriction(s) " << restriction_str << " " ;
                }
                ss << "at node " << event_context->GetId().data << "\n" ;
                LOG_INFO( ss.str().c_str() );
            }
        }
        delete qi_as_config;
        if(ndi)
        {
            ndi->Release();
        }
        tsteps_since_last = 0;
        num_repetitions--;
        if( num_repetitions == 0 )
        {
            distribution_complete = true; // we're done, signal disposal ok
        }

        //distribution_complete = true; // we're done, signal disposal ok
        // this signals each process individually that its ok to clean up, in general if the completion times might be different on different nodes 
        // we'd want to coordinate the cleanup signal in Update()
    }

    bool StandardInterventionDistributionEventCoordinator::TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec)
    {
        if (demographic_coverage == 1.0 )
        {
            return true;
        }
        double randomDraw = randgen->e();
        LOG_DEBUG_F("randomDraw = %f, demographic_coverage = %f\n", randomDraw, demographic_coverage);
        return randomDraw <= demographic_coverage;
    }

    bool
    StandardInterventionDistributionEventCoordinator::visitIndividualCallback( 
        IIndividualHumanEventContext *ihec,
        float & incrementalCostOut,
        ICampaignCostObserver * pICCO
    )
    {
        {
            // Add some arbitrary check on individual to determine if they get a bednet.
            // TODO: Demographic targeting goes here.
            // Add real checks on demographics based on intervention demographic targetting. 
            // Return immediately if we hit a non-distribute condition
            if( target_demographic != TargetDemographicType::Everyone || property_restrictions.size() ) // don't waste any more time with checks if we're giving to everyone
            {
                if( qualifiesDemographically( ihec ) == false )
                {
                    LOG_DEBUG("Individual not given intervention because not in target demographic\n");
                    return false;
                }
                property_restrictions_verified = true;
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

                // instantiate and distribute intervention
                LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", std::string(json::QuickInterpreter(intervention_config._json)["class"].As<json::String>()).c_str() );
                IDistributableIntervention *di = _di->Clone();
                release_assert(di);
                if (di)
                {
                    di->AddRef();
                    di->Distribute( ihec->GetInterventionsContext(), pICCO );
                    di->Release(); // a bit wasteful for now, could cache it for the next fellow

                    LOG_DEBUG_F("Distributed an intervention %p to individual %d at a cost of %f\n", di, ihec->GetSuid().data, incrementalCostOut);
                }
            }
        }
        return true;
    }



    void StandardInterventionDistributionEventCoordinator::regenerateCachedNodeContextPointers()
    {
        // regenerate the cached INodeEventContext* pointers fromthe cached node suids
        // the fact that this needs to happen is probably a good argument for the EC to own the NodeSet, since it needs to query the SEC for the node ids and context pointers anyway
        cached_nodes.clear();
        for (auto& node_id : node_suids)
        {
            cached_nodes.push_back(parent->GetNodeEventContext(node_id));
        }
    }

    void StandardInterventionDistributionEventCoordinator::formatInterventionClassNames( std::ostringstream& intervention_name, json::QuickInterpreter* actual_intervention_config)
    {
        if ( actual_intervention_config->Exist("Actual_Intervention_Config") )
        {
            // find actual_intervention_config if it exists
            actual_intervention_config = &( (*actual_intervention_config)["Actual_Intervention_Config"] );

            // append class name to intervention name
            intervention_name << " -> " << std::string( (*actual_intervention_config)["class"].As<json::String>() );

            // keep looking recursively for more actual_intervention_config layers
            formatInterventionClassNames( intervention_name, actual_intervention_config );
        }
        else if ( actual_intervention_config->Exist("Actual_Intervention_Configs") )
        {
            // maybe it was an array of actual interventions
            const json::Array& actual_interventions_array = (*actual_intervention_config)["Actual_Intervention_Configs"].As<json::Array>();

            // loop over array
            intervention_name << " -> ";
            int array_size = actual_interventions_array.Size();
            for( int idx = 0; idx < array_size; idx++ )
            {
                if( idx == 0 && array_size > 1 )
                    intervention_name << "[ ";
                else if ( idx > 0 )
                    intervention_name << ", ";

                // accumulate individual class names
                const json::Object& actual_intervention = json_cast<const json::Object&>( actual_interventions_array[idx] );
                actual_intervention_config = &json::QuickInterpreter(actual_intervention);
                intervention_name << std::string( (*actual_intervention_config)["class"].As<json::String>() );

                // recursively search for each for more layers!
                formatInterventionClassNames( intervention_name, actual_intervention_config );
            }
            if( array_size > 1 ) intervention_name << " ]";
        }
        else if ( actual_intervention_config->Exist("Positive_Diagnosis_Config") )
        {
            // maybe it was a diagnostic with positive_diagnosis_config if it exists (TODO: can this part be merged with the acutal_intervention_config block?)
            actual_intervention_config = &( (*actual_intervention_config)["Positive_Diagnosis_Config"] );

            // append class name to intervention name
            intervention_name << " -> " << std::string( (*actual_intervention_config)["class"].As<json::String>() );

            // keep looking recursively for more actual_intervention_config layers
            formatInterventionClassNames( intervention_name, actual_intervention_config );
        }
        else
        {
            // end of recursive call
        }
    }

    bool 
    StandardInterventionDistributionEventCoordinator::IsFinished()
    {
        return distribution_complete;
    }

    // private/protected
    bool
    StandardInterventionDistributionEventCoordinator::qualifiesDemographically(
        const IIndividualHumanEventContext * const pIndividual
    )
    const
    {
        bool retQualifies = true;

        if (target_demographic == TargetDemographicType::PossibleMothers &&
                 !pIndividual->IsPossibleMother())
        {
            LOG_DEBUG("Individual not given intervention because not possible mother\n");
            return false;
        }
        else if( target_demographic == TargetDemographicType::ExplicitAgeRanges || target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender)
        {
            if( pIndividual->GetAge() < target_age_min * DAYSPERYEAR )
            {
                LOG_DEBUG_F("Individual %lu not given intervention because too young (age=%f) for intervention min age (%f)\n", ((IndividualHuman*)pIndividual)->GetSuid().data, pIndividual->GetAge(), target_age_min* DAYSPERYEAR);
                return false;
            }
            else if( pIndividual->GetAge() > target_age_max * DAYSPERYEAR )
            {
                LOG_DEBUG_F("Individual %lu not given intervention because too old (age=%f) for intervention max age (%f)\n", ((IndividualHuman*)pIndividual)->GetSuid().data, pIndividual->GetAge(), target_age_max* DAYSPERYEAR);
                return false;
            }

            if( target_demographic == TargetDemographicType::ExplicitAgeRangesAndGender )
            {
                // Gender = 0 is MALE, Gender = 1 is FEMALE.  Should use Gender::Enum throughout code
                if( pIndividual->GetGender() == 0 && target_gender == TargetGender::Female )
                {
                    return false;
                }
                else if( pIndividual->GetGender() == 1 && target_gender == TargetGender::Male )
                {
                    return false;
                }
            }
        }
        else if (target_demographic == TargetDemographicType::ExplicitGender )
        {
            // Gender = 0 is MALE, Gender = 1 is FEMALE.  Should use Gender::Enum throughout code
            if( pIndividual->GetGender() == 0 && target_gender == TargetGender::Female )
            {
                return false;
            }
            else if( pIndividual->GetGender() == 1 && target_gender == TargetGender::Male )
            {
                return false;
            }
        }

        if( property_restrictions.size() && retQualifies )
        {
            // individual has to have one of these properties
            for (auto& prop : property_restrictions_map)
            {
                const std::string& szKey = prop.first;
                const std::string& szVal = prop.second;

                if( property_restrictions_verified == false )
                {
                    Node::VerifyPropertyDefinedInDemographics( szKey, szVal );

                    //property_restrictions_verified = true;
                }

                auto * pProp = const_cast<Kernel::IIndividualHumanEventContext*>(pIndividual)->GetProperties();
                release_assert( pProp );

                LOG_DEBUG_F( "Applying property restrictions in event coordinator: %s/%s.\n", szKey.c_str(), szVal.c_str() );
                // Every individual has to have a property value for each property key
                if( pProp->find( szKey ) == pProp->end() )
                {
                    throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", szKey.c_str() );
                }
                else if( pProp->at( szKey ) == szVal )
                {
                    continue; // we're good
                }
                else
                {
                    retQualifies = false;
                    break;
                }
            }
        }
        else
        {
            LOG_DEBUG( "No property restrictions in event coordiantor to apply.\n" );
        }
        LOG_DEBUG_F( "Returning %d from %s\n", retQualifies, __FUNCTION__ );
        return retQualifies;
    }

    float StandardInterventionDistributionEventCoordinator::GetDemographicCoverage() const
    {
        return demographic_coverage;
    }

    float
    StandardInterventionDistributionEventCoordinator::getDemographicCoverageForIndividual(
        const IIndividualHumanEventContext *pInd
    )
    const
    {
        return GetDemographicCoverage();
    }

    TargetDemographicType::Enum StandardInterventionDistributionEventCoordinator::GetTargetDemographic() const
    {
        return target_demographic;
    }

    float StandardInterventionDistributionEventCoordinator::GetMinimumAge() const
    {
        return target_age_min;
    }

    float StandardInterventionDistributionEventCoordinator::GetMaximumAge() const
    {
        return target_age_max;
    }

    void StandardInterventionDistributionEventCoordinator::ProcessDeparting(
        IIndividualHumanEventContext *pInd
    )
    {
        LOG_INFO("Individual departing from node receiving intervention. TODO: enforce demographic and other qualifiers.\n");
        float incrementalCostOut = 0.0f;
        visitIndividualCallback( pInd, incrementalCostOut, NULL /* campaign cost observer */ );
    } // these do nothing for now

    void
    StandardInterventionDistributionEventCoordinator::ProcessArriving(
        IIndividualHumanEventContext *pInd
    )
    {
        LOG_INFO("Individual arriving at node receiving intervention. TODO: enforce demographic and other qualifiers.\n");
        float incrementalCostOut = 0.0f;
        visitIndividualCallback( pInd, incrementalCostOut, NULL /* campaign cost observer */ );
    }

#if USE_JSON_SERIALIZATION

    // IJsonSerializable Interfaces
    void StandardInterventionDistributionEventCoordinator::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("coverage", coverage);
        root->Insert("distribution_complete", distribution_complete);
        root->Insert("num_repetitions", num_repetitions);
        root->Insert("tsteps_between_reps", tsteps_between_reps);
        root->Insert("demographic_coverage", demographic_coverage);
        root->Insert("target_demographic", target_demographic);
        root->Insert("target_age_min", target_age_min);
        root->Insert("target_age_max", target_age_max);
        root->Insert("include_emigrants", include_emigrants);
        root->Insert("include_immigrants", include_immigrants);
        root->Insert("tsteps_since_last", tsteps_since_last);
        root->Insert("intervention_activated", intervention_activated);

        root->Insert("intervention_config");
        intervention_config.JSerialize(root, helper);

        root->Insert("node_suids");
        root->BeginArray();
        for (auto& sid : node_suids)
        {
            sid.JSerialize(root, helper);
        }
        root->EndArray();

        root->EndObject();
    }

    void StandardInterventionDistributionEventCoordinator::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif
}
#if USE_BOOST_SERIALIZATION
// TODO: Consolidate with serialization code in header.
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT(Kernel::StandardInterventionDistributionEventCoordinator);
namespace Kernel
{

    template<class Archive>
    void serialize(Archive &ar, StandardInterventionDistributionEventCoordinator &ec, const unsigned int v)
    {
        boost::serialization::void_cast_register<StandardInterventionDistributionEventCoordinator, IEventCoordinator>();
        boost::serialization::void_cast_register<StandardInterventionDistributionEventCoordinator, ITravelLinkedDistributionSource>();
        ar & ec.coverage;
        ar & ec.distribution_complete;
        ar & ec.num_repetitions;
        ar & ec.tsteps_between_reps;
        ar & ec.demographic_coverage;
        ar & ec.target_demographic;
        ar & ec.target_age_min;
        ar & ec.target_age_max;
        ar & ec.include_emigrants;
        ar & ec.include_immigrants;
        ar & ec.tsteps_since_last;
        ar & ec.intervention_activated;
        ar & ec.intervention_config;

        // need to save the list of suids and restore from them, rather than saving the context pointers
        //ar & cached_nodes;
        ar & ec.node_suids;
    }
}

#endif
