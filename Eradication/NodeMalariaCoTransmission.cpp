
#include "stdafx.h"

#include "NodeMalariaCoTransmission.h"
#include "IndividualMalariaCoTransmission.h"
#include "VectorPopulationIndividualMalariaCoTran.h"
#include "StrainAwareTransmissionGroupsGPCoTran.h"

SETUP_LOGGING( "NodeMalariaCoTransmission" )

namespace Kernel
{
    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_DERIVED(NodeMalariaCoTransmission, NodeMalaria)
        HANDLE_INTERFACE(INodeMalariaCoTransmission)
    END_QUERY_INTERFACE_DERIVED(NodeMalariaCoTransmission, NodeMalaria)

    NodeMalariaCoTransmission::NodeMalariaCoTransmission() 
        : NodeMalaria()
        , m_HumanIdToStrainIdentityMapIndoor()
        , m_HumanIdToStrainIdentityMapOutdoor()
        , m_VectorIdToStrainIdentityMapIndoor()
        , m_VectorIdToStrainIdentityMapOutdoor()
    {
    }

    NodeMalariaCoTransmission::NodeMalariaCoTransmission(ISimulationContext *simulation, ExternalNodeId_t externalNodeId, suids::suid suid)
        : NodeMalaria(simulation, externalNodeId, suid)
        , m_HumanIdToStrainIdentityMapIndoor()
        , m_HumanIdToStrainIdentityMapOutdoor()
        , m_VectorIdToStrainIdentityMapIndoor()
        , m_VectorIdToStrainIdentityMapOutdoor()
    {
    }


    NodeMalariaCoTransmission *NodeMalariaCoTransmission::CreateNode(ISimulationContext *simulation, ExternalNodeId_t externalNodeId, suids::suid suid)
    {
        NodeMalariaCoTransmission *newnode = _new_ NodeMalariaCoTransmission(simulation, externalNodeId, suid);
        newnode->Initialize();

        return newnode;
    }

    NodeMalariaCoTransmission::~NodeMalariaCoTransmission()
    {
        ClearMap( m_HumanIdToStrainIdentityMapIndoor );
        ClearMap( m_HumanIdToStrainIdentityMapOutdoor );
        ClearMap( m_VectorIdToStrainIdentityMapIndoor );
        ClearMap( m_VectorIdToStrainIdentityMapOutdoor );
    }

    IIndividualHuman* NodeMalariaCoTransmission::createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender)
    {
        return IndividualHumanMalariaCoTransmission::CreateHuman(getContextPointer(), suid, monte_carlo_weight, initial_age, gender);
    }

    void NodeMalariaCoTransmission::ClearMap( std::map<uint32_t, StrainIdentityMalariaCoTran*>& rIdToStrainMap )
    {
        for( auto entry : rIdToStrainMap )
        {
            delete entry.second;
        }
        rIdToStrainMap.clear();
    }

    void NodeMalariaCoTransmission::updateInfectivity( float dt )
    {
        ClearMap( m_HumanIdToStrainIdentityMapIndoor );
        ClearMap( m_HumanIdToStrainIdentityMapOutdoor );
        ClearMap( m_VectorIdToStrainIdentityMapIndoor );
        ClearMap( m_VectorIdToStrainIdentityMapOutdoor );

        NodeMalaria::updateInfectivity( dt );
    }

    ITransmissionGroups* NodeMalariaCoTransmission::CreateTransmissionGroups()
    {
        txOutdoor = new StrainAwareTransmissionGroupsGPCoTran( GetRng() );
        txOutdoor->SetTag( "outdoor" );
        txIndoor = new StrainAwareTransmissionGroupsGPCoTran( GetRng() );
        txIndoor->SetTag( "indoor" );
        return nullptr; // Don't want Node::transmissionGroups used
    }

    IVectorPopulation* NodeMalariaCoTransmission::CreateVectorPopulation( VectorSamplingType::Enum vectorSamplingType,
                                                                          int speciesIndex,
                                                                          int32_t populationPerSpecies )
    {
        IVectorPopulation* pop = nullptr;
        if (vectorSamplingType == VectorSamplingType::TRACK_ALL_VECTORS || 
            vectorSamplingType == VectorSamplingType::SAMPLE_IND_VECTORS)
        {
            // Individual mosquito model
            LOG_DEBUG( "Creating VectorPopulationIndividual instance(s).\n" );
            pop = VectorPopulationIndividualMalariaCoTran::CreatePopulation(getContextPointer(), speciesIndex, populationPerSpecies, mosquito_weight);
        }
        else
        {
            const char* name = VectorSamplingType::pairs::lookup_key( vectorSamplingType );
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Malaria_Model", "MALARIA_MECHANISTIC_MODEL_WITH_CO_TRANSMISSION", "Vector_Sampling_Type", name );
        }
        return pop;
    }

    void NodeMalariaCoTransmission::DepositFromIndividual( const IStrainIdentity& strainIDs,
                                                           const GeneticProbability& contagion_quantity,
                                                           TransmissionRoute::Enum route )
    {
        NodeVector::DepositFromIndividual( strainIDs, contagion_quantity, route );

        if( contagion_quantity.GetSum() <= 0.0 )
        {
            return;
        }

        // Creating a new object that will be owned by the appropriate map
        IStrainIdentity* p_si = strainIDs.Clone();
        StrainIdentityMalariaCoTran* p_si_malaria = dynamic_cast<StrainIdentityMalariaCoTran*>(p_si);
        release_assert( p_si_malaria != nullptr );

        uint32_t entity_id = p_si_malaria->GetGeneticID(); // human or vector

        switch (route)
        {
            case TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR:
                m_HumanIdToStrainIdentityMapIndoor[ entity_id ] = p_si_malaria;
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR:
                m_HumanIdToStrainIdentityMapOutdoor[ entity_id ] = p_si_malaria;
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR:
                m_VectorIdToStrainIdentityMapIndoor[ entity_id ] = p_si_malaria;
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR:
                m_VectorIdToStrainIdentityMapOutdoor[ entity_id ] = p_si_malaria;
                break;

            default:
                // TODO - try to get proper string from route enum
                throw new BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "route", uint32_t(route), "???" );
        }
    }

    const StrainIdentityMalariaCoTran* GetStrainIdentity( const std::map<uint32_t, StrainIdentityMalariaCoTran*>& rIdToStrainMap,
                                                          uint32_t id,
                                                          const char* pVariableName )
    {
        if( rIdToStrainMap.count( id ) == 0 )
        {
            std::stringstream ss;
            ss << "id=" << id << " not found in " << pVariableName << endl;
            ss << "map has: ";
            for( auto entry : rIdToStrainMap )
            {
                ss << entry.first << ", ";
            }
            ss << endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return rIdToStrainMap.at( id );
    }

    const StrainIdentityMalariaCoTran& NodeMalariaCoTransmission::GetCoTranStrainIdentityForPerson( bool isIndoor, uint32_t personID ) const
    {
        if( isIndoor )
        {
            return *GetStrainIdentity( m_HumanIdToStrainIdentityMapIndoor,
                                       personID,
                                       "m_HumanIdToStrainIdentityMapIndoor" );
        }
        else
        {
            return *GetStrainIdentity( m_HumanIdToStrainIdentityMapOutdoor,
                                       personID,
                                       "m_HumanIdToStrainIdentityMapOutdoor" );
        }
    }

    const StrainIdentityMalariaCoTran& NodeMalariaCoTransmission::GetCoTranStrainIdentityForVector( bool isIndoor, uint32_t vectorID ) const
    {
        if( isIndoor )
        {
            return *GetStrainIdentity( m_VectorIdToStrainIdentityMapIndoor,
                                       vectorID,
                                       "m_VectorIdToStrainIdentityMapIndoor" );
        }
        else
        {
            return *GetStrainIdentity( m_VectorIdToStrainIdentityMapOutdoor,
                                       vectorID,
                                       "m_VectorIdToStrainIdentityMapOutdoor" );
        }
    }

    void NodeMalariaCoTransmission::VectorBitPerson( bool isIndoor, uint32_t vectorID )
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! We are commenting this out so that we don't remove the vector.  This makes
        // !!! the model like the base model (even exact InsetCharts).  It allows one vector to
        // !!! bite multiple people in one day.  The vector only gets "one feed", but gets to
        // !!! infect potentially more than one person.  You also get the down side of an 
        // !!! infectious vector getting a feed according to the vector model but not when it
        // !!! gets to the human side.  This allows us to match the base model that can generate
        // !!! more new infections than the vector model determined.  This helps to avoid
        // !!! premature elimination of infectious vectors and elimination of the parasite.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        // **************************************************************************************
        // *** NOTE:  If you UNCOMMENT this, there is still a bug about with regards to which
        // *** humans get which bites.  Right now, the logic can run out of bites before it is
        // *** done processing all of the humans.
        // **************************************************************************************

        // ----------------------------------------------------------------------------
        // --- At this point, the contagion from the vector is now in the human group
        // --- due to how EndUpate() uses the ScalingMatrix to transfer the contagion
        // --- deposited by the vector group and make it available to the human group.
        // ----------------------------------------------------------------------------
        //StrainIdentity si;
        //si.SetGeneticID( vectorID );
        //txIndoor->ClearStrain( &si, NodeVector::human_indoor );
        //txOutdoor->ClearStrain( &si, NodeVector::human_outdoor );
    }

    REGISTER_SERIALIZABLE(NodeMalariaCoTransmission);

    void NodeMalariaCoTransmission::serialize(IArchive& ar, NodeMalariaCoTransmission* obj)
    {
        NodeMalaria::serialize(ar, obj);
        NodeMalariaCoTransmission& node = *obj;

        // These should be transiant and should not need saving
        //ar.labelElement( "m_HumanIdToStrainIdentityMapIndoor"   ) & node.m_HumanIdToStrainIdentityMapIndoor;
        //ar.labelElement( "m_HumanIdToStrainIdentityMapOutdoor"  ) & node.m_HumanIdToStrainIdentityMapOutdoor;
        //ar.labelElement( "m_VectorIdToStrainIdentityMapIndoor"  ) & node.m_VectorIdToStrainIdentityMapIndoor;
        //ar.labelElement( "m_VectorIdToStrainIdentityMapOutdoor" ) & node.m_VectorIdToStrainIdentityMapOutdoor;
    }
}
