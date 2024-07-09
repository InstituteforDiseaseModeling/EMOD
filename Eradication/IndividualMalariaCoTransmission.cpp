
#include "stdafx.h"

#include <numeric> //for std::accumulate
#include "IndividualMalariaCoTransmission.h"
#include "NodeMalariaCoTransmission.h"
#include "StrainIdentityMalariaCoTran.h"
#include "EventTrigger.h"
#include "Vector.h"
#include "RANDOM.h"
#include "Exceptions.h"

SETUP_LOGGING( "IndividualMalariaCoTransmission" )

namespace Kernel
{
    // ----------------- IndividualHumanMalariaCoTransmission ---------------
    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanMalariaCoTransmission, IndividualHumanMalaria)
        HANDLE_INTERFACE(IMalariaHumanReport)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanMalariaCoTransmission, IndividualHumanMalaria)

    IndividualHumanMalariaCoTransmission::IndividualHumanMalariaCoTransmission(suids::suid _suid, double monte_carlo_weight, double initial_age, int gender)
        : IndividualHumanMalaria(_suid, monte_carlo_weight, initial_age, gender)
        , m_pNodeCoTran( nullptr )
        , m_VectorToHumanStrainIdentity()
        , m_strain_indoors()
    {
    }

    IndividualHumanMalariaCoTransmission::IndividualHumanMalariaCoTransmission(INodeContext *context)
        : IndividualHumanMalaria(context)
        , m_pNodeCoTran( nullptr )
        , m_VectorToHumanStrainIdentity()
        , m_strain_indoors()
    {
    }

    IndividualHumanMalariaCoTransmission *IndividualHumanMalariaCoTransmission::CreateHuman(INodeContext *context, suids::suid id, double weight, double initial_age, int gender)
    {
        IndividualHumanMalariaCoTransmission *newhuman = _new_ IndividualHumanMalariaCoTransmission(id, weight, initial_age, gender);

        newhuman->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newhuman->m_age );

        return newhuman;
    }

    IndividualHumanMalariaCoTransmission::~IndividualHumanMalariaCoTransmission()
    {
    }

    void IndividualHumanMalariaCoTransmission::SetContextTo(INodeContext* context)
    {
        IndividualHumanMalaria::SetContextTo(context);

        // It can be null when emigrating.
        if( context != nullptr )
        {
            if( s_OK != context->QueryInterface( GET_IID( INodeMalariaCoTransmission ), (void**)&m_pNodeCoTran ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "m_context", "INodeMalariaCoTransmission", "INodeContext" );
            }
        }
    }

    void IndividualHumanMalariaCoTransmission::ExposeToInfectivity(float dt, TransmissionGroupMembership_t transmissionGroupMembership)
    {
        // ---------------------------------------------------------------------------
        // --- m_strain_indoors goes with m_strain_exposure.  We clear it here but it
        // --- gets updated in AddExposure().  We need it to keep track of whether or
        // --- not the exposure was indoors.  This gets used to determine which map the
        // --- StrainIdentityMalariaCoTran is located in.
        // ---------------------------------------------------------------------------
        m_strain_indoors.clear();
        IndividualHumanMalaria::ExposeToInfectivity( dt, transmissionGroupMembership );
        m_strain_indoors.clear(); // clear after using it so infections from OutbreakIndividual don't get confused.
    }

    void IndividualHumanMalariaCoTransmission::AddExposure( const StrainIdentity& rStrainId,
                                                            float totalExposure,
                                                            TransmissionRoute::Enum route )
    {
        IndividualHumanMalaria::AddExposure( rStrainId, totalExposure, route );

        // ------------------------------------------------------------------------
        // --- set whether the strain is indoor or outdoors so it can be used later
        // --- when calling GetCoTranStrainIdentityForVector()
        // ------------------------------------------------------------------------
        bool is_indoors = (route == TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR);
        m_strain_indoors.push_back( std::make_pair( rStrainId, is_indoors ) );
    }

    void IndividualHumanMalariaCoTransmission::AcquireNewInfection( const IStrainIdentity *infstrain,
                                                                    int incubation_period_override )
    {
        // Prepare the data structure for the reporter
        m_VectorToHumanStrainIdentity.Clear();
        m_VectorToHumanStrainIdentity.SetAcquiredPersonID( GetSuid().data );

        uint32_t vector_id = 0;
        bool is_indoor = false;

        // m_strain_indoors.size() = 0 if coming from OutbreakIndividual
        if( m_strain_indoors.size() > 0 )
        {
            // the vector id should be in the genetic id
            vector_id = infstrain->GetGeneticID();

            // Get flag indicating if bite was indoors or outdoors
            is_indoor = false;
            bool found = false;
            for( const auto& entry : m_strain_indoors )
            {
                if( entry.first.GetGeneticID() == infstrain->GetGeneticID() )
                {
                    is_indoor = entry.second;
                    found = true;
                    break;
                }
            }
            release_assert( found );

            // Get full data from transmission group
            const StrainIdentityMalariaCoTran& r_si_malaria = m_pNodeCoTran->GetCoTranStrainIdentityForVector( is_indoor, vector_id );

            m_VectorToHumanStrainIdentity.SetTransmittedData( r_si_malaria );
        }

        IInfection* p_infection_previous = m_pNewInfection;
        StrainIdentity si;
        IndividualHumanMalaria::AcquireNewInfection( &si );

        // ------------------------------------------------------------------------------------------------
        // --- m_pNewInfection can be null if the person already had the maximum number of infections
        // --- before possibly getting this one.  A person could get more than one infection per time step
        // --- by getting one via OutbreakIndividual and another by transmission.  If m_pNewInfection
        // --- changes value, then the person got the infection and it was not stopped by max infections.
        // ------------------------------------------------------------------------------------------------
        if( (m_pNewInfection != nullptr) && (m_pNewInfection != p_infection_previous) )
        {
            m_VectorToHumanStrainIdentity.AddAcquiredInfection( m_pNewInfection->GetSuid().data );

            if( m_strain_indoors.size() > 0 )
            {
                // vector bit a person so remove it from the pool so it cannot bite another person
                m_pNodeCoTran->VectorBitPerson( is_indoor, vector_id );
            }
            broadcaster->TriggerObservers( GetEventContext(), EventTrigger::VectorToHumanTransmission );
        }
    }

    void IndividualHumanMalariaCoTransmission::DepositInfectiousnessFromGametocytes()
    {
        float weighted_infectiousnesss = GetWeightedInfectiousness();
        if( weighted_infectiousnesss == 0.0 )
        {
            return;
        }

        // ------------------------------------------------------------------------------
        // --- NOTE: Each vector is first determining if they could have gotten infected
        // --- from the human population.  Hence, if everyone had a good vaccine, then the
        // --- vector might not get infected, or if the gametocytes have not matured so
        // --- the vector can't get any.  Once the vector determines that she gets infected,
        // --- she picks a person to get infected from.
        // --- Also note, that each person deposits their amount of infectiousness/contagion.
        // --- Each of these values gets divided by the total population of the node.
        // --- The sum of these values is the probability that the vector gets infected.
        // --- Each individual probability is the likelihood that the particular person
        // --- was the one that distributed the contagion.
        // ------------------------------------------------------------------------------

        // ------------------------------------------------------------------------------
        // --- Set first arg of genetic ID to the human id so that vectors will select a
        // --- a person to bite via ResolveInfectingStrain().  Set the second argument to
        // --- keep track of who transmitted the infection.  Then update this object
        // --- with information about the infections that the person has.  This data can be
        // --- used by the post processing for transmission tracking.
        // ------------------------------------------------------------------------------
        StrainIdentityMalariaCoTran strain( GetSuid().data, GetSuid().data );
        for( auto p_inf : this->infections )
        {
            IInfectionMalaria* p_inf_malaria = nullptr;
            if( p_inf->QueryInterface(GET_IID(IInfectionMalaria), (void**)&p_inf_malaria) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "p_inf", "IInfectionMalaria", "IInfection" );
            }

            // ignore infections that don't have any mature gametocytes, typically new ones.
            if( p_inf_malaria->get_mature_gametocyte_density() > 0.0 )
            {
                // for our purposes infection = strain
                strain.AddTransmittedInfection( p_inf->GetSuid().data,
                                                p_inf_malaria->get_mature_gametocyte_density() );
            }
        }

        INodeVector* p_node_vector = nullptr;
        if ( s_OK != parent->QueryInterface(GET_IID(INodeVector), (void**)&p_node_vector) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "INodeVector", "INodeContext" );
        }

        // Effects from vector intervention container
        IVectorInterventionsEffects* ivie = GetVectorInterventionEffects();

        p_node_vector->DepositFromIndividual( strain,
                                              ivie->GetblockIndoorVectorTransmit()*weighted_infectiousnesss,
                                              TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_INDOOR );
        p_node_vector->DepositFromIndividual( strain,
                                              ivie->GetblockOutdoorVectorTransmit()*weighted_infectiousnesss,
                                              TransmissionRoute::TRANSMISSIONROUTE_HUMAN_TO_VECTOR_OUTDOOR );

    }

    const StrainIdentityMalariaCoTran& IndividualHumanMalariaCoTransmission::GetRecentTransmissionInfo() const
    {
        return m_VectorToHumanStrainIdentity;
    }

    REGISTER_SERIALIZABLE(IndividualHumanMalariaCoTransmission);

    void IndividualHumanMalariaCoTransmission::serialize(IArchive& ar, IndividualHumanMalariaCoTransmission* obj)
    {
        IndividualHumanMalaria::serialize(ar, obj);
        IndividualHumanMalariaCoTransmission& individual = *obj;

        // This should be temporary so should not need to be saved
        //ar.labelElement( "m_strain_indoors" ) & individual.m_strain_indoors;

        // This is just contains temporary data for event reporting - don't need to save
        //ar.labelElement("m_VectorToHumanStrainIdentity") & individual.m_VectorToHumanStrainIdentity;
    }
}

