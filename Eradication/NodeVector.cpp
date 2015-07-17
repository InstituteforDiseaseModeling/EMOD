/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include <sstream> // for std::ostringstream
#include <boost/lexical_cast.hpp> // for std::ostringstream
#include "Debug.h"
#include "Exceptions.h"
#include "NodeVector.h"
#include "NodeVectorEventContext.h"
#include "IndividualVector.h"
#include "VectorCohortIndividual.h"
#include "VectorPopulationIndividual.h"
#include "VectorPopulationAging.h"
#include "Log.h"
#include "SimulationConfig.h"
#include "TransmissionGroupsFactory.h"
#include "TransmissionGroupMembership.h"

static const char * _module = "NodeVector";

#ifdef randgen
#undef randgen
#endif
#include "RANDOM.h"
#define randgen (parent->GetRng())

using namespace std;

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(NodeVector, Node)
        HANDLE_INTERFACE(IVectorNodeContext)
        HANDLE_INTERFACE(INodeVector)
    END_QUERY_INTERFACE_DERIVED(NodeVector, Node)

    GET_SCHEMA_STATIC_WRAPPER_IMPL(NodeVector, NodeVector)

    TransmissionGroupMembership_t NodeVector::human_to_vector_all;
    TransmissionGroupMembership_t NodeVector::human_to_vector_indoor;
    TransmissionGroupMembership_t NodeVector::human_to_vector_outdoor;
    TransmissionGroupMembership_t NodeVector::vector_to_human_all;
    TransmissionGroupMembership_t NodeVector::vector_to_human_indoor;
    TransmissionGroupMembership_t NodeVector::vector_to_human_outdoor;

    NodeVector::NodeVector() 
        : Node()
        , larval_habitat_multiplier()
    {
    }

    NodeVector::NodeVector(ISimulationContext *context, suids::suid _suid) 
        : Node(context, _suid)
        , larval_habitat_multiplier()
    {
    }

    bool
    NodeVector::Configure(
        const Configuration * config
    )
    {
        initConfigTypeMap( "Enable_Vector_Migration", &vector_migration, Enable_Vector_Migration_DESC_TEXT, false );
        initConfigTypeMap( "Enable_Vector_Migration_Wind", &vector_migration_wind, Enable_Vector_Migration_Wind_DESC_TEXT, false );
        initConfigTypeMap( "Enable_Vector_Migration_Human", &vector_migration_human, Enable_Vector_Migration_Human_DESC_TEXT, false );
        initConfigTypeMap( "Enable_Vector_Migration_Local", &vector_migration_local, Enable_Vector_Migration_Local_DESC_TEXT, false );
        initConfigTypeMap( "Mosquito_Weight", &mosquito_weight, Mosquito_Weight_DESC_TEXT, 1, 1e4, 1 ); // should this be renamed vector_weight?
        return Node::Configure( config );
    }

    void NodeVector::Initialize()
    {
        Node::Initialize();

        if (ClimateFactory::climate_structure == ClimateStructure::CLIMATE_OFF)
        {
            // This could be either a vector sim or a malaria sim. Let's get the correct sim type for the error message.
            const char* simulation_type_name = SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type );
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Structure", "ClimateStructure::CLIMATE_OFF", "Simulation_Type", simulation_type_name );
        }

        m_vector_lifecycle_probabilities = VectorProbabilities::CreateVectorProbabilities();
    }

    void NodeVector::SetParameters(NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory)
    {
        Node::SetParameters(demographics_factory, climate_factory);

        if (demographics["NodeAttributes"].Contains("LarvalHabitatMultiplier"))
        {
            if( demographics["NodeAttributes"]["LarvalHabitatMultiplier"].IsObject() )
            {
                const char** habitat_keys = VectorHabitatType::pairs::get_keys();
                std::vector<std::string> habitats(habitat_keys,habitat_keys+VectorHabitatType::pairs::count());
                for (auto habitat_name : habitats) // TODO: JsonObjectDemog::Iterator with Enum checking inside loop?
                {
                    if (demographics["NodeAttributes"]["LarvalHabitatMultiplier"].Contains(habitat_name))
                    {
                        VectorHabitatType::Enum habitat=(VectorHabitatType::Enum) VectorHabitatType::pairs::lookup_value(habitat_name.c_str());
                        larval_habitat_multiplier[habitat] = (float)(demographics["NodeAttributes"]["LarvalHabitatMultiplier"][habitat_name].AsDouble());
                        LOG_INFO_F("Node ID=%d with LarvalHabitatMultiplier(%s)=%0.2f\n", externalId, habitat_name.c_str(), larval_habitat_multiplier[habitat]);
                    }
                    else
                    {
                        LOG_DEBUG_F("No LarvalHabitatMultiplier specified for %s habitat at Node ID=%d\n",habitat_name.c_str(),externalId);
                    }
                }
            }
            else
            {
                float multiplier = (float)(demographics["NodeAttributes"]["LarvalHabitatMultiplier"].AsDouble());
                larval_habitat_multiplier[VectorHabitatType::ALL_HABITATS] = multiplier;
                LOG_INFO_F("Node ID=%d with LarvalHabitatMultiplier(ALL_HABITATS)=%0.2f\n", externalId, multiplier);
                LOG_WARN("DeprecationWarning: Specification of \"LarvalHabitatMultiplier\" as a floating-point value in the \"NodeAttributes\" block will soon be deprecated. Specify as an object with habitat-type keys, e.g. \"LarvalHabitatMultiplier\" : {\"TEMPORARY_RAINFALL\" : 0.3}\n");
            }
        }
        else
        {
            LOG_DEBUG("Did not find the LarvalHabitatMultiplier property in NodeAttributes.\n");
        }
    }

    void NodeVector::setupEventContextHost()
    {
        event_context_host = _new_ NodeVectorEventContextHost(this);
    }

    NodeVector *NodeVector::CreateNode(ISimulationContext *context, suids::suid suid)
    {
        NodeVector *newnode = _new_ NodeVector(context, suid);
        newnode->Initialize();

        return newnode;
    }

    NodeVector::~NodeVector()
    {
        for (auto population : m_vectorpopulations)
        {
            delete population;
        }
        m_vectorpopulations.clear();

        for (auto habitat : m_larval_habitats)
        {
            delete habitat;
        }
        m_larval_habitats.clear();
    }

    IndividualHuman *NodeVector::createHuman(suids::suid id, float MCweight, float init_age, int gender, float init_poverty)
    {
        return IndividualHumanVector::CreateHuman(getContextPointer(), id, MCweight, init_age, gender, init_poverty);
    }

    IndividualHuman* NodeVector::processImmigratingIndividual(IndividualHumanVector* movedind)
    {
        Node::processImmigratingIndividual(movedind);

        return movedind;
    }

    IndividualHuman *NodeVector::addNewIndividual(float MCweight, float init_age, int gender, int init_infs, float immparam, float riskparam, float mighet, float init_poverty)
    {
        // just the base class for now
        return Node::addNewIndividual(MCweight, init_age, gender, init_infs, immparam, riskparam, mighet, init_poverty);
    }

    void NodeVector::SetupIntranodeTransmission()
    {
        // create two routes (indoor and outdoor), and two groups for each route (human, mosquito).
        // future expansion of heterogeneous intra-node transmission could start here.
        transmissionGroups = TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::HumanVectorGroups);
        string propertyAndRouteName("Indoor");
        PropertyValueList_t valueList;
        valueList.push_back(string("Human"));
        valueList.push_back(string("Vector"));

        MatrixRow_t rowOne(2, 0.0f);
        rowOne[0] = 1.0f;
        ScalingMatrix_t scalingMatrix;
        scalingMatrix.push_back(rowOne);
        MatrixRow_t rowTwo(2, 0.0f);
        rowTwo[1] = 1.0f;
        scalingMatrix.push_back(rowTwo);
        transmissionGroups->AddProperty(propertyAndRouteName, valueList, scalingMatrix, propertyAndRouteName);
        
        propertyAndRouteName = "Outdoor";
        transmissionGroups->AddProperty(propertyAndRouteName, valueList, scalingMatrix, propertyAndRouteName);

        LOG_DEBUG("groups added.\n");

        VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE(SimulationConfig)->vector_sampling_type;
        if ( (vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_NUMBER || vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_PERCENT) &&
              params()->number_basestrains > 1 &&
              params()->number_substrains  > 1 )
        {
            // Ideally error messages would all be in Exceptions.cpp
            std::ostringstream msg;
            msg << "Strain tracking is only fully supported for the individual (not cohort) vector model."
                << " Simulations will run for cohort models, but all vector-to-human transmission defaults to strain=(0,0)." 
                << " Specified values for number_basestrains = " << params()->number_basestrains << " and number_substrains = " << params()->number_substrains
                << " are not allowed. They may only be set to 1 for the cohort model.  To use the individual vector model, switch vector sampling type to TRACK_ALL_VECTORS or SAMPLE_IND_VECTORS."
                << std::endl;
            LOG_ERR( msg.str().c_str() );
            throw IncoherentConfigurationException(
                __FILE__, __LINE__, __FUNCTION__,
                "params()->number_basestrains  && params()->number_substrains > 1",
                (boost::lexical_cast<string>(params()->number_basestrains) + " && " + boost::lexical_cast<string>(params()->number_substrains)).c_str(),
                "vector_sampling_type",
                VectorSamplingType::pairs::lookup_key( vector_sampling_type )
            );
        }

        LOG_DEBUG("Building indoor/outdoor human-vector groups.\n");

        RouteToContagionDecayMap_t decayMap; 
        decayMap.clear();
        decayMap["Indoor"] = 1.0f;
        decayMap["Outdoor"] = 1.0f;
        transmissionGroups->Build(decayMap, params()->number_basestrains, params()->number_substrains);

        // Do once for all nodes
        RouteList_t route_all;
        RouteList_t route_indoor;
        RouteList_t route_outdoor;
        route_indoor.push_back("Indoor");
        route_all.push_back("Indoor");
        route_outdoor.push_back("Outdoor");
        route_all.push_back("Outdoor");

        tProperties vectorProperties;
        tProperties humanProperties;
        vectorProperties[string("Indoor")] = string("Vector");
        vectorProperties[string("Outdoor")] = string("Vector");
        humanProperties[string("Indoor")] = string("Human");
        humanProperties[string("Outdoor")] = string("Human");

        GetGroupMembershipForIndividual( route_all, &humanProperties, &human_to_vector_all );
        GetGroupMembershipForIndividual( route_all, &vectorProperties, &vector_to_human_all );
        GetGroupMembershipForIndividual( route_indoor, &humanProperties, &human_to_vector_indoor );
        GetGroupMembershipForIndividual( route_outdoor, &humanProperties, &human_to_vector_outdoor );
        GetGroupMembershipForIndividual( route_indoor, &vectorProperties, &vector_to_human_indoor );
        GetGroupMembershipForIndividual( route_outdoor, &vectorProperties, &vector_to_human_outdoor );
    }

    void NodeVector::updateInfectivity(float dt)
    {
        vector<float> infectivity_correction;

        // update population stats
        updatePopulationStatistics(dt);

        // normalize human-to-vector contagion (per-bite infection probabilities)
        // This is the place in this pattern to scale by returning mortality: (1-p_kill_PFV)
        // TODO: it should be possible to push the PFV normalization back into VectorProbabilities
        INodeVectorInterventionEffects* invie = NULL;
        if (s_OK != GetEventContext()->QueryInterface(GET_IID(INodeVectorInterventionEffects), (void**)&invie))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, "GetEventContext()", "INodeVectorInterventionEffects", "INodeEventContext" );
        }
        float weight = 1.0f - invie->GetPFVKill();

        // Acquire infections with strain tracking for exposed queues
        transmissionGroups->CorrectInfectivityByGroup(weight, &NodeVector::human_to_vector_all);

        // changes in larval capacities.
        // drying of larval habitat, function of temperature and humidity
        for (auto habitat : m_larval_habitats)
        {
            release_assert(habitat);
            habitat->Update( dt, getContextPointer() );
        }

        transmissionGroups->EndUpdate(1.0f);

        // don't need to update the vector populations before the first timestep
        if ( dt <= 0 ) 
            return;

        // process vector populations and get infection rate
        infectionrate = 0;
        for (auto population : m_vectorpopulations)
        {
            release_assert(population);
            population->UpdateVectorPopulation(dt);
            infectionrate += (float) ( population->getInfectivity() );
        }

        // Now process the node's emigrating mosquitoes
        processEmigratingVectors();
    }

    void NodeVector::updateVectorLifecycleProbabilities(float dt)
    {
        // Reset vector lifecycle transition probabilities
        m_vector_lifecycle_probabilities->ResetProbabilities();
        IVectorInterventionsEffects* ivie = NULL;

        for (auto individual : individualHumans)
        {
            IIndividualHumanVectorContext* host_individual = NULL;
            // We want IndividualHUman base class to give us a blessed pointer to IndivudalHumanVector
            if( individual->QueryInterface( GET_IID( IIndividualHumanVectorContext ), (void**)&host_individual ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanVectorContext", "IndividualHuman" );
            }

            // Host vector weight includes heterogeneous biting
            // effect of heterogeneous biting explored in Smith, D. L., F. E. McKenzie, et al. (2007). "Revisiting the basic reproductive number for malaria and its implications for malaria control." PLoS Biol 5(3): e42.
            // also in Smith, D. L., J. Dushoff, et al. (2005). "The entomological inoculation rate and Plasmodium falciparum infection in African children." Nature 438(7067): 492-495.
            float host_vector_weight = (float) ( individual->GetMonteCarloWeight() * host_individual->GetRelativeBitingRate() );

            // Query for individual vector intervention effects interface
            IIndividualHumanInterventionsContext* context = individual->GetInterventionsContext();
            if (s_OK != context->QueryInterface(GET_IID(IVectorInterventionsEffects), (void**)&ivie))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "host_individual->GetInterventionsContext()", "IVectorInterventionsEffects", "IIndividualHumanInterventionsContext" );
            }

            // Accumulate the probabilities corresponding to individual vector intervention effects
            m_vector_lifecycle_probabilities->AccumulateIndividualProbabilities(ivie, host_vector_weight);
        }

        // Normalize lifecycle probabilities by population weight
        m_vector_lifecycle_probabilities->NormalizeIndividualProbabilities();

        // Query for node vector intervention effects interface
        INodeVectorInterventionEffects* invie = NULL;
        if (s_OK !=  GetEventContext()->QueryInterface(GET_IID(INodeVectorInterventionEffects), (void**)&invie))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "GetEventContext()", "INodeVectorInterventionEffects", "INodeEventContext" );
        }

        // Get effects of node-level interventions on vector lifecycle probabilities
        m_vector_lifecycle_probabilities->SetNodeProbabilities(invie, dt);
    }

    void NodeVector::updatePopulationStatistics(float dt)
    {
        // Update population statistics as in the base class
        Node::updatePopulationStatistics(dt);

        // Additionally normalize "Human Infectious Reservoir" reporting channel variable
        // to be consistent with previous behavior, where it is the human-to-vector infection probability
        mInfectivity = (statPop > 0) ? mInfectivity/statPop : 0;

        // Another loop through the individuals to calculate the vector lifecycle probabilities
        updateVectorLifecycleProbabilities(dt);
    }

    void NodeVector::PopulateFromDemographics()
    {
        // Add the vector populations
        SetVectorPopulations();

        // Populate the people as in the base class
        Node::PopulateFromDemographics();
    }

    void NodeVector::SetVectorPopulations()
    {
        if (suid.data <= 0)
        {
            int _id = suid.data;
            LOG_WARN_F("Node suid value is %d\n", _id); // EAW: throw exception?
            return;
        }

        // Individual mosquito model
        VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE(SimulationConfig)->vector_sampling_type;
        if (vector_sampling_type == VectorSamplingType::TRACK_ALL_VECTORS || 
            vector_sampling_type == VectorSamplingType::SAMPLE_IND_VECTORS)
        {
            LOG_DEBUG( "Creating VectorPopulationIndividual instance(s).\n" );
            for (auto& vector_species_name : params()->vector_species_names)
            {
                VectorPopulation *vectorpopulation = VectorPopulationIndividual::CreatePopulation(getContextPointer(), vector_species_name, DEFAULT_VECTOR_POPULATION_SIZE, 0, mosquito_weight);
                InitializeVectorPopulation(vectorpopulation);
            }
        }
        // Aging cohort model
        else if (params()->vector_aging)
        {
            LOG_DEBUG( "Creating VectorPopulationAging instance(s).\n" );
            for (auto& vector_species_name : params()->vector_species_names)
            {
                VectorPopulation *vectorpopulation = VectorPopulationAging::CreatePopulation(getContextPointer(), vector_species_name);
                InitializeVectorPopulation(vectorpopulation);
            }
        }
        // Cohort model
        else
        {
            LOG_DEBUG( "Creating VectorPopulation (regular) instance(s).\n" );
            for (auto& vector_species_name : params()->vector_species_names)
            {
                VectorPopulation *vectorpopulation = VectorPopulation::CreatePopulation(getContextPointer(), vector_species_name);
                InitializeVectorPopulation(vectorpopulation);
            }
        }
    }

    void NodeVector::AddVectors(std::string releasedSpecies, VectorMatingStructure _vector_genetics, unsigned long int releasedNumber)
    {
        bool found_existing_vector_population = false;
        for (auto population : m_vectorpopulations)
        {
            if(population->get_SpeciesID() == releasedSpecies)
            {
                found_existing_vector_population = true;
                population->AddVectors(_vector_genetics, releasedNumber);
                break;
            }
        }

        if ( !found_existing_vector_population )
        {
            throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__, "Adding vectors (e.g. through a MosquitoRelease intervention) with a species name other than an existing VectorPopulation is not yet supported." );
        }

        return;
    }

    void NodeVector::processImmigratingVector(VectorCohort* immigrant)
    {
        // NodeVector probably shouldn't assume that all VectorCohorts are
        // VectorCohortIndividuals... true for now, but may not be in the future

        // Looping over each population to check if the species ID is the same is a bit lame
        // But propagating a list-to-map container change for typically 1-3 species is probably overkill
        // Given that the conditional takes less time than the AddAdults line in performance profiling
        IVectorCohortIndividual* vci = NULL;
        if( immigrant->QueryInterface( GET_IID( IVectorCohortIndividual ), (void**) &vci ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "immigrant", "IVectorCohortIndividual", "VectorCohort" );
        }
        for (auto population : m_vectorpopulations)
        {
            if ( population->get_SpeciesID() == vci->GetSpecies() )
            {
                population->AddAdults(immigrant);
                return;
            }
        }
    }

    void NodeVector::processEmigratingVectors()
    {
        // to hold the total migration rate summed over all destination nodes
        float vectormigrationrate = 0;

        // vector of node suids for adjacent nodes with compatible (i.e. local) migration type
        std::vector<suids::suid> vectormigCommIDs(0);

        // calculate vectormigrationrate
        if (vector_migration_local && migration_info)
        {
            std::vector<int /*MigrationType*/> migration_types = migration_info->GetMigrationTypes();
            std::vector<suids::suid> adjacent_nodes = migration_info->GetAdjacentNodes();

            for (int i = 0; i < adjacent_nodes.size(); i++)
            {
                if (migration_types[i] == LOCAL_MIGRATION)
                {
                    // increase total vector emigration rate
                    if (params()->lloffset > 0)
                    {
                        // .0045 is the lloffset for the 30 arc second grid.  This allows bigger cells to lose fewer mosquitoes to diffusion
                        vectormigrationrate = (float)(vectormigrationrate + 0.125 * 0.5 * 0.0045 / params()->lloffset);   // 50 percent leave a 1km x 1 km square per day
                    }
                    else
                    {
                        // If there is no spatial scale associated with the grid, then only 10 percent of mosquitoes leave the node
                        vectormigrationrate = (float)(vectormigrationrate + 0.125 * 0.5 * 0.2);
                    }

                    // locally adjacent nodes
                    vectormigCommIDs.push_back(adjacent_nodes[i]);
                }
            }
        }

        if (vector_migration_wind)  {} // adjust for wind
        if (vector_migration_human) {} // adjust for other migration routes

        // bookkeeping
        VectorCohortList_t migratingvectors;             // to hold vectors
        VectorCohort *tempentry;                         // to hold vector popped off migrating list
        std::vector<suids::suid>::iterator itNodeId;     // iterator for "sprinkling" vectors evenly across locally adjacent nodes

        if (vectormigrationrate > 0)
        {
            IVectorSimulationContext* ivsc = context();
            for (auto population : m_vectorpopulations)
            {
                population->Vector_Migration(vectormigrationrate, &migratingvectors);

                // now apportion migratingvectors list for each species to destination nodes (evenly for now, since only local migration)
                itNodeId = vectormigCommIDs.begin();
                while (migratingvectors.size() > 0)
                {
                    tempentry = migratingvectors.front();
                    migratingvectors.pop_front();

                    // give vectors to attached communities like a sprinkler
                    tempentry->SetMigrationDestination(*itNodeId);
                    ivsc->PostMigratingVector(tempentry);

                    // circular iteration among available adjacent nodes
                    if ( ++itNodeId == vectormigCommIDs.end() )
                    {
                        itNodeId = vectormigCommIDs.begin();
                    }
                }
            }
        }
    }

    IVectorSimulationContext* NodeVector::context() const
    {
        IVectorSimulationContext *ivsc;
        if (s_OK != parent->QueryInterface(GET_IID(IVectorSimulationContext), (void**)&ivsc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IVectorSimulationContext", "IVectorSimulationContext" );
        }

        return ivsc;
    }

    void NodeVector::propagateContextToDependents()
    {
        Node::propagateContextToDependents();

        for (auto population : m_vectorpopulations)
        {
            population->SetContextTo(getContextPointer());
        }
    }

    const SimulationConfig*
    NodeVector::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    VectorProbabilities* NodeVector::GetVectorLifecycleProbabilities()
    {
        return m_vector_lifecycle_probabilities;
    }

    VectorHabitat* NodeVector::GetVectorHabitatByType(VectorHabitatType::Enum type)
    {
        VectorHabitatList_t::iterator it = std::find_if( m_larval_habitats.begin(), m_larval_habitats.end(), [type](VectorHabitat* habitat){ return habitat->GetVectorHabitatType() == type; } );
        if ( it != m_larval_habitats.end() )
        { 
            LOG_DEBUG_F("Found larval habitat with type = %s\n", VectorHabitatType::pairs::lookup_key(type));
            return *it;
        }
        else
        {
            LOG_DEBUG_F("There is no larval habitat yet with type = %s\n", VectorHabitatType::pairs::lookup_key(type));
            return NULL;
        }
    }

    void NodeVector::AddVectorHabitat(VectorHabitat* habitat)
    {
        m_larval_habitats.push_front(habitat);
    }

    float NodeVector::GetLarvalHabitatMultiplier(VectorHabitatType::Enum type) const
    {
        return HabitatMultiplierByType(type)*HabitatMultiplierByType(VectorHabitatType::ALL_HABITATS);
    }

    float NodeVector::HabitatMultiplierByType(VectorHabitatType::Enum type) const
    {
        auto it=larval_habitat_multiplier.find(type);
        if (it==larval_habitat_multiplier.end())
        {
            LOG_DEBUG_F("No modifiers for habitat type %s\n",VectorHabitatType::pairs::lookup_key(type));
            return 1.0f;
        }
        else
        {
            float scale=it->second;
            LOG_DEBUG_F("Habitat scale modified by %0.2f for type %s\n",scale,VectorHabitatType::pairs::lookup_key(type));
            return scale;
        }
    }

    void NodeVector::InitializeVectorPopulation(VectorPopulation* vp)
    {
        // Link vector population to group collection to allow it to update vector-to-human infectivity
        // TBD: QI socialNetwork to IWVPC pointer

        vp->SetupIntranodeTransmission(transmissionGroups);

        // Create larval habitat objects for this species
        // and keep a list of pointers so the node can update it with new rainfall, etc.
        vp->SetupLarvalHabitat(getContextPointer());

        // Add this new vector population to the list
        m_vectorpopulations.push_front(vp);
    }

    VectorPopulationList_t& NodeVector::GetVectorPopulations()
    {
        return m_vectorpopulations;
    }
} // end namespace Kernel

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::NodeVector)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, NodeVector& node, const unsigned int  file_version )
    {
        static const char * _module = "NodeVector";
        LOG_DEBUG("(De)serializing NodeVector\n");

        // Register derived types
        ar.template register_type<Kernel::IndividualHumanVector>();
        // Just commenting these out to see if it gets stuff to build
        //ar.template register_type<Kernel::VectorPopulationAging>();
        //ar.template register_type<Kernel::VectorPopulationIndividual>();
        ar.template register_type<Kernel::NodeEventContextHost>();

        // Serialize fields
        ar & node.m_vectorpopulations;
        ar & node.m_larval_habitats;
        ar & node.larval_habitat_multiplier

        // Serialize base class
        ar & boost::serialization::base_object<Kernel::Node>(node);
    }
}
#endif
