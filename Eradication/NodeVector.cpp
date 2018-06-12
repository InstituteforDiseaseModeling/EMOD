/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <sstream> // for std::ostringstream
#include <boost/lexical_cast.hpp> // for std::ostringstream
#include "Debug.h"
#include "Exceptions.h"
#include "NodeVector.h"
#include "NodeVectorEventContext.h"
#include "IndividualVector.h"
#include "VectorPopulation.h"
#include "VectorPopulationIndividual.h"
#include "VectorParameters.h"
#include "Log.h"
#include "SimulationConfig.h"
#include "TransmissionGroupsFactory.h"
#include "TransmissionGroupMembership.h"
#include "IMigrationInfoVector.h"

SETUP_LOGGING( "NodeVector" )

#ifdef randgen
#undef randgen
#endif
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
        : m_larval_habitats()
        , m_vectorpopulations()
        , m_VectorPopulationReportingList()
        , m_vector_lifecycle_probabilities( nullptr )
        , larval_habitat_multiplier()
        , vector_mortality( true )
        , mosquito_weight( 0 )
        , vector_migration_info( nullptr )
    {
        delete event_context_host;
        NodeVector::setupEventContextHost();    // This is marked as a virtual function, but isn't virtualized here because we're still in the ctor.
    }

    NodeVector::NodeVector(ISimulationContext *context, suids::suid _suid) 
        : Node(context, _suid)
        , m_larval_habitats()
        , m_vectorpopulations()
        , m_VectorPopulationReportingList()
        , m_vector_lifecycle_probabilities( nullptr )
        , larval_habitat_multiplier()
        , vector_mortality( true )
        , mosquito_weight( 1 )
        , vector_migration_info( nullptr )
    {
        delete event_context_host;
        NodeVector::setupEventContextHost();    // This is marked as a virtual function, but isn't virtualized here because we're still in the ctor.
    }

    bool
    NodeVector::Configure(
        const Configuration * config
    )
    {
        larval_habitat_multiplier.Initialize();

        initConfigTypeMap( "Enable_Vector_Mortality", &vector_mortality, Enable_Vector_Mortality_DESC_TEXT, true );
        initConfigTypeMap( "Mosquito_Weight", &mosquito_weight, Mosquito_Weight_DESC_TEXT, 1, 1e4, 1, "Vector_Sampling_Type", "SAMPLE_IND_VECTORS" ); // should this be renamed vector_weight?

        bool configured = Node::Configure( config );

        return configured;
    }

    void NodeVector::Initialize()
    {
        Node::Initialize();


        if (ClimateFactory::climate_structure == ClimateStructure::CLIMATE_OFF)
        {
            // This could be either a vector sim or a malaria sim. Let's get the correct sim type for the error message.
            const char* simulation_type_name = SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type );
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Climate_Model", "ClimateStructure::CLIMATE_OFF", "Simulation_Type", simulation_type_name );
        }

        m_vector_lifecycle_probabilities = VectorProbabilities::CreateVectorProbabilities();
    }

    void NodeVector::SetParameters( NodeDemographicsFactory *demographics_factory, ClimateFactory *climate_factory, bool white_list_enabled )
    {
        Node::SetParameters( demographics_factory, climate_factory, white_list_enabled );

        if (demographics["NodeAttributes"].Contains("LarvalHabitatMultiplier"))
        {
            larval_habitat_multiplier.Read( demographics["NodeAttributes"]["LarvalHabitatMultiplier"].GetJsonObject(), externalId );
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
        m_VectorPopulationReportingList.clear();

        for (auto& entry : m_larval_habitats)
        {
            LOG_DEBUG_F("%s: Cleaning up habitats for species '%s'.\n", __FUNCTION__, entry.first.c_str() );
            for (auto& habitat : entry.second)
            {
                delete habitat;
                habitat = nullptr;
            }
        }
        m_larval_habitats.clear();

        delete vector_migration_info ;
    }

    IIndividualHuman* NodeVector::createHuman( suids::suid id, float MCweight, float init_age, int gender)
    {
        return IndividualHumanVector::CreateHuman(getContextPointer(), id, MCweight, init_age, gender);
    }

    IIndividualHuman* NodeVector::processImmigratingIndividual( IIndividualHuman* movedind)
    {
        Node::processImmigratingIndividual(movedind);

        return movedind;
    }

    IIndividualHuman* NodeVector::addNewIndividual( float MCweight, float init_age, int gender, int init_infs, float immparam, float riskparam, float mighet)
    {
        // just the base class for now
        return Node::addNewIndividual(MCweight, init_age, gender, init_infs, immparam, riskparam, mighet);
    }

    void NodeVector::SetupMigration( IMigrationInfoFactory * migration_factory,
                                     MigrationStructure::Enum ms,
                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap )
    {
        Node::SetupMigration( migration_factory, ms, rNodeIdSuidMap );

        IMigrationInfoFactoryVector* p_mf_vector = dynamic_cast<IMigrationInfoFactoryVector*>(migration_factory);
        release_assert( p_mf_vector );

        vector_migration_info = p_mf_vector->CreateMigrationInfoVector( this, rNodeIdSuidMap );
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

        VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_sampling_type;
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
            std::ostringstream err_msg;
            err_msg << params()->number_basestrains << " and " << params()->number_substrains;
            throw IncoherentConfigurationException(
                __FILE__, __LINE__, __FUNCTION__,
                "params()->number_basestrains > 1 and params()->number_substrains > 1",
                err_msg.str().c_str(),
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

// Workaround (AKA hack) for deserialization
for (auto pop : m_vectorpopulations)
{
    pop->SetupIntranodeTransmission(transmissionGroups);
}
    }

    void NodeVector::updateInfectivity(float dt)
    {
        vector<float> infectivity_correction;

        // update population stats
        updatePopulationStatistics(dt);

        // normalize human-to-vector contagion (per-bite infection probabilities)
        // This is the place in this pattern to scale by returning mortality: (1-p_kill_PFV)
        // TODO: it should be possible to push the PFV normalization back into VectorProbabilities
        INodeVectorInterventionEffects* invie = nullptr;
        if (s_OK != GetEventContext()->QueryInterface(GET_IID(INodeVectorInterventionEffects), (void**)&invie))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "GetEventContext()", "INodeVectorInterventionEffects", "INodeEventContext" );
        }
        float weight = 1.0f - invie->GetPFVKill();

        // Acquire infections with strain tracking for exposed queues
        transmissionGroups->CorrectInfectivityByGroup(weight, &NodeVector::human_to_vector_all);

        // changes in larval capacities.
        // drying of larval habitat, function of temperature and humidity
        for ( auto& entry : m_larval_habitats )
        {
            LOG_DEBUG_F( "%s: Updating habitats for mosquito species %s.\n", __FUNCTION__, entry.first.c_str() );

            for ( auto habitat : entry.second )
            {
                release_assert( habitat );
                habitat->Update( dt, getContextPointer(), entry.first );
            }
        }

        transmissionGroups->EndUpdate(1.0f); // finish processing human-to-mosquito infectiousness

        // don't need to update the vector populations before the first timestep
        if ( dt <= 0 ) 
            return;

        // process vector populations and get infection rate
        infectionrate = 0;
        for (auto population : m_vectorpopulations)
        {
            release_assert(population);
            population->UpdateVectorPopulation(dt);

            IVectorPopulationReporting* p_ivpr = nullptr;
            if( population->QueryInterface( GET_IID( IVectorPopulationReporting ), (void**)&p_ivpr ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "population", "IVectorPopulationReporting", "IVectorPopulation" );
            }
            infectionrate += float( p_ivpr->getInfectivity());
        }

        // do again so that humans bitten this time step get infected
        transmissionGroups->EndUpdate( 1.0f ); // finish processing mosquito-to-human infectiousness

        // Now process the node's emigrating mosquitoes
        processEmigratingVectors( dt );
    }

    void NodeVector::updateVectorLifecycleProbabilities(float dt)
    {
        // Reset vector lifecycle transition probabilities
        m_vector_lifecycle_probabilities->ResetProbabilities();
        IVectorInterventionsEffects* ivie = nullptr;

        for (auto individual : individualHumans)
        {
            IIndividualHumanVectorContext* host_individual = nullptr;
            // We want IndividualHUman base class to give us a blessed pointer to IndivudalHumanVector
            if( individual->QueryInterface( GET_IID( IIndividualHumanVectorContext ), (void**)&host_individual ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanVectorContext", "IndividualHuman" );
            }

            // Host vector weight includes heterogeneous biting
            // effect of heterogeneous biting explored in Smith, D. L., F. E. McKenzie, et al. (2007). "Revisiting the basic reproductive number for malaria and its implications for malaria control." PLoS Biol 5(3): e42.
            // also in Smith, D. L., J. Dushoff, et al. (2005). "The entomological inoculation rate and Plasmodium falciparum infection in African children." Nature 438(7067): 492-495.
            float host_vector_weight = float(individual->GetMonteCarloWeight() * host_individual->GetRelativeBitingRate());

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
        INodeVectorInterventionEffects* invie = nullptr;
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
        VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE( SimulationConfig )->vector_params->vector_sampling_type;

        for( auto& vector_species_name : params()->vector_params->vector_species_names )
        {
            int32_t population_per_species = DEFAULT_VECTOR_POPULATION_SIZE;
            if( demographics[ "NodeAttributes" ].Contains( "InitialVectorsPerSpecies" ) )
            {
                if( demographics[ "NodeAttributes" ][ "InitialVectorsPerSpecies" ].IsObject() )
                {
                    if( demographics[ "NodeAttributes" ][ "InitialVectorsPerSpecies" ].Contains( vector_species_name ) )
                    {
                        population_per_species = demographics[ "NodeAttributes" ][ "InitialVectorsPerSpecies" ][ vector_species_name ].AsInt();
                    }
                }
                else
                {
                    population_per_species = demographics[ "NodeAttributes" ][ "InitialVectorsPerSpecies" ].AsInt();
                }
            }
            LOG_DEBUG_F( "population_per_species = %f.\n", population_per_species );

            VectorPopulation *vectorpopulation = nullptr;

            if (vector_sampling_type == VectorSamplingType::TRACK_ALL_VECTORS || 
                vector_sampling_type == VectorSamplingType::SAMPLE_IND_VECTORS)
            {
                // Individual mosquito model
                LOG_DEBUG( "Creating VectorPopulationIndividual instance(s).\n" );
                vectorpopulation = VectorPopulationIndividual::CreatePopulation(getContextPointer(), vector_species_name, population_per_species, 0, mosquito_weight);
            }
            else
            {
                // Cohort model
                LOG_DEBUG( "Creating VectorPopulation (regular) instance(s).\n" );
                vectorpopulation = VectorPopulation::CreatePopulation( getContextPointer(), vector_species_name, population_per_species, 0 );
            }

            release_assert( vectorpopulation );
            InitializeVectorPopulation( vectorpopulation );
        }

    }

    void NodeVector::AddVectors( const std::string& releasedSpecies, const VectorMatingStructure& _vector_genetics, uint32_t releasedNumber)
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

    void NodeVector::processImmigratingVector( IVectorCohort* immigrant )
    {
        // Looping over each population to check if the species ID is the same is a bit lame
        // But propagating a list-to-map container change for typically 1-3 species is probably overkill
        // Given that the conditional takes less time than the AddAdults line in performance profiling

        const std::string& vci_species = immigrant->GetSpecies();
        for (auto population : m_vectorpopulations)
        {
            const std::string& pop_species = population->get_SpeciesID();
            auto pop_length = pop_species.size();
            if (vci_species.size() == pop_length)
            {
                if (memcmp(pop_species.c_str(), vci_species.c_str(), pop_length) == 0)
                {
                    population->AddImmigratingVector(immigrant);
                    return;
                }
            }
        }

        std::stringstream ss;
        ss << "Should have found population for species=" << vci_species;
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }

    void NodeVector::processEmigratingVectors( float dt )
    {
        if( !vector_migration_info )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "migration is not configured correctly" );
        }

        if( vector_migration_info->GetReachableNodes().size() > 0 )
        {
            VectorMigrationBasedOnFiles( dt );
        }
    }

    void NodeVector::VectorMigrationBasedOnFiles( float dt )
    {
        IVectorSimulationContext* ivsc = context();
        release_assert( vector_migration_info );

        VectorCohortVector_t migrating_vectors;
        for( auto vp : m_vectorpopulations )
        {
            vector_migration_info->UpdateRates( this->GetSuid(), vp->get_SpeciesID(),  ivsc );

            migrating_vectors.clear();
            vp->Vector_Migration( dt, vector_migration_info, &migrating_vectors );
            for (auto p_vc : migrating_vectors)
            {
                ivsc->PostMigratingVector(this->GetSuid(), p_vc);
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
            population->SetupIntranodeTransmission(transmissionGroups);
        }

        if ( vector_migration_info != nullptr )
        {
            vector_migration_info->SetContextTo( getContextPointer() );
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

    IVectorHabitat* NodeVector::GetVectorHabitatBySpeciesAndType( std::string& species, VectorHabitatType::Enum type, const Configuration* inputJson )
    {
        IVectorHabitat* habitat;

        VectorHabitatList_t& habitats = m_larval_habitats[ species ];
        VectorHabitatList_t::iterator it = std::find_if( habitats.begin(), habitats.end(), [type](IVectorHabitat* entry){ return entry->GetVectorHabitatType() == type; } );
        if ( it != habitats.end() )
        { 
            LOG_DEBUG_F( "%s: Found larval habitat with type = %s\n", __FUNCTION__, VectorHabitatType::pairs::lookup_key( type ) );
            habitat = *it;
        }
        else
        {
            release_assert( inputJson != nullptr );

            LOG_DEBUG_F( "%s: Creating new larval habitat with type %s for species %s.\n", __FUNCTION__, VectorHabitatType::pairs::lookup_key( type ), species.c_str() );
            habitat = VectorHabitat::CreateHabitat( type, inputJson );
            // Previous code used push_front() but the habitats were updated in VectorPopulation which kept a list
            // populated with push_back() so we'll use push_back() to maintain the same order of update.
            habitats.push_back( habitat );
        }

        return habitat;
    }

    VectorHabitatList_t* NodeVector::GetVectorHabitatsBySpecies( std::string& species )
    {
        // This will create an empty habitat list if necessary, which is okay.
        return &(m_larval_habitats[ species ]);
    }

    float NodeVector::GetLarvalHabitatMultiplier(VectorHabitatType::Enum type, const std::string& species) const
    {
        return larval_habitat_multiplier.GetMultiplier( type, species ) *
               larval_habitat_multiplier.GetMultiplier( VectorHabitatType::ALL_HABITATS, species );
    }

    void NodeVector::InitializeVectorPopulation( IVectorPopulation* vp )
    {
        // Link vector population to group collection to allow it to update vector-to-human infectivity
        // TBD: QI socialNetwork to IWVPC pointer

        vp->SetupIntranodeTransmission(transmissionGroups);

        // Create larval habitat objects for this species
        // and keep a list of pointers so the node can update it with new rainfall, etc.
        vp->SetupLarvalHabitat(getContextPointer());

        vp->SetVectorMortality( vector_mortality );

        // Add this new vector population to the list
        m_vectorpopulations.push_front(vp);

        IVectorPopulationReporting* p_ivpr = nullptr;
        if( vp->QueryInterface( GET_IID( IVectorPopulationReporting ), (void**)&p_ivpr ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "vp", "IVectorPopulationReporting", "IVectorPopulation" );
        }
        m_VectorPopulationReportingList.push_front( p_ivpr );
    }

    const VectorPopulationReportingList_t& NodeVector::GetVectorPopulationReporting() const
    {
        return m_VectorPopulationReportingList;
    }

    REGISTER_SERIALIZABLE(NodeVector);

    void NodeVector::serialize(IArchive& ar, NodeVector* obj)
    {
        Node::serialize(ar, obj);
        NodeVector& node = *obj;

        if ((node.serializationMask & SerializationFlags::Population) != 0) {
            ar.labelElement("m_larval_habitats") & node.m_larval_habitats;
            ar.labelElement("m_vectorpopulations") & node.m_vectorpopulations;

            ar.labelElement("m_vector_lifecycle_probabilities"); VectorProbabilities::serialize( ar, node.m_vector_lifecycle_probabilities );

            if( ar.IsReader() )
            {
                for (const auto vector_population : node.m_vectorpopulations)
                {
                    IVectorPopulationReporting* p_ivpr = nullptr;
                    if( vector_population->QueryInterface( GET_IID( IVectorPopulationReporting ), (void**)&p_ivpr ) != s_OK )
                    {
                        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "vector_population", "IVectorPopulationReporting", "IVectorPopulation" );
                    }
                    node.m_VectorPopulationReportingList.push_back( p_ivpr );
                }
            }
        }

        if ((node.serializationMask & SerializationFlags::Parameters) != 0) {
// clorton            ar.labelElement("larval_habitat_multiplier") & node.larval_habitat_multiplier;
            ar.labelElement("vector_mortality") & node.vector_mortality;
            ar.labelElement("mosquito_weight") & node.mosquito_weight;
        }

        if ((node.serializationMask & SerializationFlags::Properties) != 0) {
        }

    }
} // end namespace Kernel
