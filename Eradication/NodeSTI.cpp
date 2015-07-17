/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "NodeSTI.h"
#include "Debug.h"

#include "IndividualSTI.h"
#include "Relationship.h"
#include "RelationshipManagerFactory.h"
#include "RelationshipGroups.h"
#include "SimulationConfig.h"

#include "SocietyFactory.h"

static const char * _module = "NodeSTI";

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(NodeSTI,NodeSTI)

    BEGIN_QUERY_INTERFACE_DERIVED(NodeSTI, Node)
        HANDLE_INTERFACE(INodeSTI)
        HANDLE_INTERFACE(IConfigurable)
    END_QUERY_INTERFACE_DERIVED(NodeSTI, Node)

    bool NodeSTI::Configure( const Configuration* config )
    {
        // A PFA burnin of at least 1 day is required to ensure transmission
        initConfigTypeMap( "PFA_Burnin_Duration_In_Days", &pfa_burnin_duration, PFA_Burnin_Duration_In_Days_DESC_TEXT, 1, FLT_MAX, 1000 * DAYSPERYEAR );

        bool ret = society->Configure( config );
        if( ret )
        {
            ret = Node::Configure( config );
        }

        if( ret &&
            !JsonConfigurable::_dryrun &&
            (ind_sampling_type != IndSamplingType::TRACK_ALL     ) &&
            (ind_sampling_type != IndSamplingType::FIXED_SAMPLING) )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                    "Individual_Sampling_Type", IndSamplingType::pairs::lookup_key(ind_sampling_type),
                                                    "Simulation_Type", SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type ),
                                                    "Relationship-based transmission network only works with 100% sampling."
                                                    );
        }

        return ret ;
    }

    NodeSTI::NodeSTI(ISimulationContext *_parent_sim, suids::suid node_suid)
        : Node(_parent_sim, node_suid)
        , relMan(nullptr)
        , society(nullptr)
        , migratedIndividualToRelationshipIdMap()
        , pfa_burnin_duration( 15 * DAYSPERYEAR )
    {
        relMan = RelationshipManagerFactory::CreateManager( this );
        society = SocietyFactory::CreateSociety( relMan );
    }

    NodeSTI::NodeSTI()
        : Node()
        , relMan(nullptr)
        , society(nullptr)
        , migratedIndividualToRelationshipIdMap()
        , pfa_burnin_duration( 15 * DAYSPERYEAR )
    {
        relMan = RelationshipManagerFactory::CreateManager( this );
        society = SocietyFactory::CreateSociety( relMan );
    }

    NodeSTI::~NodeSTI(void)
    {
        delete society ;
        delete relMan ;
    }

    NodeSTI *NodeSTI::CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid)
    {
        NodeSTI *newnode = _new_ NodeSTI(_parent_sim, node_suid);
        newnode->Initialize();

        return newnode;
    }

    void NodeSTI::Initialize()
    {
        Node::Initialize();
    }

    void NodeSTI::SetMonteCarloParameters(float indsamplerate, int nummininf)
    {
        if( indsamplerate != 1.0f )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                   "Base_Individual_Sampling_Rate", std::to_string( indsamplerate ).c_str(),
                                                   "Simulation_Type", SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type ),
                                                   "Relationship-based transmission network only works with 100% sampling."
                                                  );
        }
        return Node::SetMonteCarloParameters( indsamplerate, nummininf );
    }

    void NodeSTI::SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory)
    {
        Node::SetParameters( demographics_factory, climate_factory );

        const std::string SOCIETY_KEY( "Society" );
        if( !demographics.Contains( SOCIETY_KEY ) )
        {
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Could not find the 'Society' element in the demographics data." );
        }
        std::istringstream iss( demographics[SOCIETY_KEY].ToString() );
        Configuration* p_config = Configuration::Load( iss, "demographics" );
        society->SetParameters( p_config );
        delete p_config ;
    }

    IndividualHuman *NodeSTI::createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender,  float above_poverty)
    {
        return IndividualHumanSTI::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender, above_poverty);
    }

    void NodeSTI::SetupIntranodeTransmission()
    {
        RelationshipGroups * relNodePools = dynamic_cast<RelationshipGroups*>(TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::RelationshipGroups));
        relNodePools->SetParent( this );
        transmissionGroups = relNodePools;
        RouteToContagionDecayMap_t decayMap;
        decayMap[string("contact")] = 1.0f;
        routes.push_back(string("contact"));
        transmissionGroups->Build(decayMap, 1, 1);
    }

    void NodeSTI::Update( float dt )
    {
        // Update relationships (dissolution only, at this point)
        list<IIndividualHuman*> population;  //not used by RelationshipManager
        relMan->Update( population, transmissionGroups, dt );

        society->BeginUpdate();

        for (auto& person : individualHumans)
        {
            IIndividualHumanSTI* sti_person = nullptr;
            if (person->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_person) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "person", "IIndividualHumanSTI", "IIndividualHuman" );
            }
            sti_person->UpdateEligibility();        // DJK: Could be slow to do this on every update.  Could check for relationship status changes. <ERAD-1869>
        }

        if (pfa_burnin_duration > 0)
        {
            pfa_burnin_duration -= dt;
            society->UpdatePairFormationRates( GetTime(), dt );
        }

        for (auto& person : individualHumans)
        {
            IIndividualHumanSTI* sti_person = nullptr;
            if (person->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_person) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "person", "IIndividualHumanSTI", "IIndividualHuman" );
            }
            sti_person->ConsiderRelationships(dt);
        }

        society->UpdatePairFormationAgents( GetTime(), dt );

        {
            RouteToContagionDecayMap_t decayMap;
            decayMap[string("contact")] = 1.0f; 
            transmissionGroups->Build( decayMap, 1, 1 );
        }

        Node::Update( dt );
    }

    act_prob_vec_t NodeSTI::DiscreteGetTotalContagion(const TransmissionGroupMembership_t* membership)
    {
        return transmissionGroups->DiscreteGetTotalContagion(membership);
    }

    /*const?*/ IRelationshipManager*
    NodeSTI::GetRelationshipManager() /*const?*/
    {
        return relMan;
    }

    ISociety*
    NodeSTI::GetSociety()
    {
        return society;
    }

    void
    NodeSTI::processEmigratingIndividual(
        IndividualHuman *individual
    )
    {
#ifdef MIGRATION_SUPPORTED
        // If this person is in relationships, store a map from individual
        // id to relationships, so that we can repop the relationship when 
        // they return.
        release_assert( individual );

        ((IndividualHumanSTI*)individual)->onEmigrating();

        IIndividualHumanSTI* sti_individual;
        if (individual->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_individual) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualSTI", "IndividualHuman" );
        }
        auto& relationships = sti_individual->GetRelationships();
        LOG_DEBUG_F( "%s: %lu with %d relationships.\n", __FUNCTION__, individual->GetSuid().data, relationships.size() );
        for (auto iterator = relationships.begin(); iterator != relationships.end(); /**/)
        {
            auto relationship = *iterator++;
            LOG_DEBUG_F( "Storing fact that invidual %lu was in relationship %d at node %lu before they migrated. Will restore on return if relationship still exists.\n",
                         individual->GetSuid().data,
                         relationship->GetId(),
                         GetSuid().data
                       );
            migratedIndividualToRelationshipIdMap.insert( std::make_pair( individual->GetSuid().data, relationship->GetId() ) );
            switch (relationship->GetType())
            {
            case RelationshipType::TRANSITORY:
                // Transitory relationships don't last through a migration.
                // They are expected to end permanently when the person leaves the Node.
                relationship->terminate( relMan );
                delete relationship;
                break;

            default:
                // More permanent relationships can last/stay open until the person comes back
                sti_individual->VacateRelationship( relationship );
                break;
            }
        }

        release_assert( relationships.size() == 0 );

        Node::processEmigratingIndividual( individual );
#endif
    }

    IndividualHuman*
    NodeSTI::processImmigratingIndividual(
        IndividualHuman* movedind
    )
    {
        auto retVal = Node::processImmigratingIndividual( movedind );
        auto movedId = movedind->GetSuid().data;

        // if returning, restore broken relationships 
        auto count = migratedIndividualToRelationshipIdMap.count( movedId );
        if ( count == 0 ) {
            LOG_DEBUG_F("No relationships found to restore for %lu at node %lu.\n", movedId, GetSuid().data);
        }

        // grab one of the relationships from the migratedIndividual multimap
        auto relationship = migratedIndividualToRelationshipIdMap.find( movedId );

        // iterate while there are relationships for movedId in the map
        while (relationship != migratedIndividualToRelationshipIdMap.end() )
        {
            unsigned int relId = relationship->second;

            LOG_DEBUG_F( "Restore %lu to relationship %d at node %lu.\n", movedId, relId, GetSuid().data );
            if( relMan->GetRelationshipById( relId ) != NULL )
            {
                IIndividualHumanSTI* sti_individual = nullptr;
                if (movedind->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_individual) != s_OK)
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "movedind", "IIndividualSTI", "IndividualHuman" );
                }
                sti_individual->RejoinRelationship( relMan->GetRelationshipById( relId ));
            }
            else
            {
                LOG_DEBUG_F( "Looks like relationship ended while you were away.\n" );
            }

            // erase this relationship from the migratedIndividualToRelationshipIdMap
            migratedIndividualToRelationshipIdMap.erase(relationship);

            // grab the next relationship from the migratedIndividual multimap
            relationship = migratedIndividualToRelationshipIdMap.find( movedId );
        }

        // when we're done here, an individual should no longer have any relationships stored in the migratedIndividual map
        release_assert( migratedIndividualToRelationshipIdMap.count( movedId ) == 0);

        return retVal;
    }
}

#if USE_BOOST_SERIALIZATION
#include "IndividualSTI.h"
BOOST_CLASS_EXPORT(Kernel::NodeSTI)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, NodeSTI& node, const unsigned int file_version)
    {
        // Register derived types
        //ar.template register_type<IndividualHumanSTI>();

        // Serialize base class
        ar &boost::serialization::base_object<Node>(node);    
    }
}
#endif
