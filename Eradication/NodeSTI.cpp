/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "NodeSTI.h"
#include "Debug.h"

#include "IndividualSTI.h"
#include "RelationshipManagerFactory.h"
#include "RelationshipGroups.h"
#include "SimulationConfig.h"

#include "SocietyFactory.h"
#include "IIdGeneratorSTI.h"
#include "NodeEventContextHost.h"
#include "ISTISimulationContext.h"

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
        , pfa_burnin_duration( 15 * DAYSPERYEAR )
    {
        relMan = RelationshipManagerFactory::CreateManager( this );
        society = SocietyFactory::CreateSociety( relMan );
    }

    NodeSTI::NodeSTI()
        : Node()
        , relMan(nullptr)
        , society(nullptr)
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
        society->SetParameters( dynamic_cast<IIdGeneratorSTI*>(parent), p_config );
        delete p_config ;
    }

    IIndividualHuman* NodeSTI::createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender,  float above_poverty)
    {
        return IndividualHumanSTI::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender, above_poverty);
    }

    void NodeSTI::SetupIntranodeTransmission()
    {
        //RelationshipGroups * relNodePools = dynamic_cast<RelationshipGroups*>(TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::RelationshipGroups));
        RelationshipGroups * relNodePools = _new_ RelationshipGroups;
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
        IIndividualHuman* individual
    )
    {
        event_context_host->TriggerNodeEventObservers( individual->GetEventContext(), IndividualEventTriggerType::STIPreEmigrating );

        IIndividualHumanSTI* sti_individual=nullptr;
        if (individual->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_individual) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualSTI", "IndividualHuman" );
        }

        sti_individual->onEmigrating();

        Node::processEmigratingIndividual( individual );
    }

    IIndividualHuman*
    NodeSTI::processImmigratingIndividual(
        IIndividualHuman* movedind
    )
    {
        // -------------------------------------------------------------------------------
        // --- SetContextTo() is called in Node::processImmigratingIndividual() but
        // --- we need need to set context before onImmigrating().  onImmigrating() needs
        // --- the RelationshipManager which is part of the node.
        // -------------------------------------------------------------------------------
        movedind->SetContextTo(getContextPointer());

        IIndividualHumanSTI* sti_individual = nullptr;
        if (movedind->QueryInterface(GET_IID(IIndividualHumanSTI), (void**)&sti_individual) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "retVal", "IIndividualSTI", "IndividualHuman" );
        }
        sti_individual->onImmigrating();

        auto retVal = Node::processImmigratingIndividual( movedind );

        event_context_host->TriggerNodeEventObservers( retVal->GetEventContext(), IndividualEventTriggerType::STIPostImmigrating );

        return retVal;
    }

    REGISTER_SERIALIZABLE(NodeSTI);

    void NodeSTI::serialize(IArchive& ar, NodeSTI* obj)
    {
        Node::serialize(ar, obj);
        // NodeSTI& node = *obj;
        // clorton TODO
    }
}
