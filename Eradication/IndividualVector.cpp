/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "IndividualVector.h"
#include "SusceptibilityVector.h"
#include "InfectionVector.h"
#include "ITransmissionGroups.h"
#include "Common.h"
#include "Debug.h"
#include "IContagionPopulation.h"
#include "TransmissionGroupMembership.h"
#include "NodeVector.h"

#include "VectorInterventionsContainer.h"

#include "Log.h"

#include "RapidJsonImpl.h" // render unnecessary when the deserializing wrapper is done


#ifdef randgen
#undef randgen
#endif
#include "RANDOM.h"
#define randgen (parent->GetRng())

static const char* _module = "IndividualVector";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanVector, IndividualHuman)
        HANDLE_INTERFACE(IIndividualHumanVectorContext)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanVector, IndividualHuman)

    IndividualHumanVector::IndividualHumanVector(suids::suid _suid, double monte_carlo_weight, double initial_age, int gender, double initial_poverty)
    : Kernel::IndividualHuman(_suid, (float)monte_carlo_weight, (float)initial_age, gender, (float)initial_poverty)
    , vector_susceptibility(NULL)
    , vector_interventions(NULL)
    {
    }

    IndividualHumanVector::IndividualHumanVector(INodeContext *context)
    : Kernel::IndividualHuman(context)
    , vector_susceptibility(NULL)
    , vector_interventions(NULL)
    {
    }

    IndividualHumanVector *IndividualHumanVector::CreateHuman(INodeContext *context, suids::suid id, double MCweight, double init_age, int gender, double init_poverty)
    {
        Kernel::IndividualHumanVector *newhuman = _new_ Kernel::IndividualHumanVector(id, MCweight, init_age, gender, init_poverty);

        newhuman->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newhuman->m_age );

        return newhuman;
    }

    IndividualHumanVector::~IndividualHumanVector()
    {
        // deletion of individuals handled by parent destructor
        // deletion of susceptibility handled by parent destructor
    }

    bool IndividualHumanVector::Configure( const Configuration * config )
    {
        LOG_DEBUG( "Configure\n" );

        SusceptibilityVectorConfig fakeImmunity;
        fakeImmunity.Configure( config );

        return JsonConfigurable::Configure( config );
    }

    void IndividualHumanVector::PropagateContextToDependents()
    {
        IndividualHuman::PropagateContextToDependents();

        if( vector_susceptibility == NULL && susceptibility != NULL)
        {
            if ( s_OK != susceptibility->QueryInterface(GET_IID(IVectorSusceptibilityContext), (void**)&vector_susceptibility) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "susceptibility", "IVectorSusceptibilityContext", "Susceptibility" );
            }
        }
    }

    void IndividualHumanVector::setupInterventionsContainer()
    {
        vector_interventions = _new_ VectorInterventionsContainer();
        interventions = vector_interventions; // initialize base class pointer to same object
    }

    void IndividualHumanVector::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        SusceptibilityVector *newsusceptibility = SusceptibilityVector::CreateSusceptibility(this, this->m_age, imm_mod, risk_mod);
        vector_susceptibility = newsusceptibility;
        susceptibility = newsusceptibility; // initialize base class pointer to same object

        susceptibility->SetContextTo(this);
    }


    void IndividualHumanVector::ExposeToInfectivity(float dt, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        // Reset counters
        m_strain_exposure.clear();
        m_total_exposure = 0;

        // Expose individual to all pools in weighted collection (i.e. indoor + outdoor)
        LOG_DEBUG("Exposure to contagion: vector to human.\n");
        parent->ExposeIndividual((IInfectable*)this, &NodeVector::vector_to_human_all, dt);

        // Decide based on total exposure to infectious bites
        // whether the individual becomes infected and with what strain
        if ( m_total_exposure > 0 )
        {
            ApplyTotalBitingExposure();
        }
    }

    void IndividualHumanVector::UpdateGroupPopulation(float size_changes)
    {
        //update nodepool population for both human-vector and vector-human since we use the same normalization for both

        parent->UpdateTransmissionGroupPopulation(&NodeVector::human_to_vector_all, size_changes, this->GetMonteCarloWeight());

//        parent->UpdateNodePoolPopulation(&NodeVector::vector_to_human_all, size_changes, this->GetMonteCarloWeight());

 
        LOG_DEBUG_F("updated population for both human and vector, with size change %f and monte carlo weight %f.\n", size_changes, this->GetMonteCarloWeight());
    }

    void IndividualHumanVector::ApplyTotalBitingExposure()
    {
        // Make random draw whether to acquire new infection
        // dt incorporated already in ExposeIndividual function arguments
        float acquisition_probability = (float)EXPCDF(-m_total_exposure);
        if ( randgen->e() >= acquisition_probability ) return;
            
        // Choose a strain based on a weighted draw over values from all vector-to-human pools
        float strain_cdf_draw = randgen->e() * m_total_exposure;
        std::vector<strain_exposure_t>::iterator it = std::lower_bound( m_strain_exposure.begin(), m_strain_exposure.end(), strain_cdf_draw, compare_strain_exposure_float_less()); 
        AcquireNewInfection(&(it->first));
    }

    void IndividualHumanVector::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route )
    {
        release_assert( cp );
        release_assert( susceptibility );
        release_assert( interventions );
#if 1
        // get rid of this. but seems to be needed for malaria garki. :( :( :(
        if( !vector_interventions )
        {
            vector_interventions = static_cast<VectorInterventionsContainer*>(interventions);
        }
#endif
        release_assert( vector_interventions );
        float acqmod = GetRelativeBitingRate() * susceptibility->getModAcquire() * interventions->GetInterventionReducedAcquire();

        switch( transmission_route )
        {
            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_INDOOR:
                acqmod *= vector_interventions->GetblockIndoorVectorAcquire();
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_VECTOR_TO_HUMAN_OUTDOOR:
                acqmod *= vector_interventions->GetblockOutdoorVectorAcquire();
                break;
        
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "transmission_route", transmission_route, "Stringified enum value not available" );
        }

        // Accumulate vector of pairs of strain ids and cumulative infection probability
        float infection_probability = cp->GetTotalContagion() * acqmod * dt;
        if ( infection_probability > 0 )
        {
            // Increment total exposure
            m_total_exposure += infection_probability;

            // With a weighted random draw, pick a strain from the ContagionPopulation CDF
            StrainIdentity strain_id;
            strain_id.SetAntigenID(cp->GetAntigenId());
            cp->ResolveInfectingStrain(&strain_id); 

            // Push this exposure and strain back to the storage array for all vector-to-human pools (e.g. indoor, outdoor)
            m_strain_exposure.push_back( std::make_pair(strain_id, m_total_exposure) );
        }
    }

    void IndividualHumanVector::UpdateInfectiousness(float dt)
    {
        infectiousness = 0;
        float tmp_infectiousness = 0;  

        typedef std::map< StrainIdentity, float >     strain_infectivity_map_t;
        typedef strain_infectivity_map_t::value_type  strain_infectivity_t;
        strain_infectivity_map_t infectivity_by_strain;
        StrainIdentity tmp_strainIDs;

        // Loop once over all infections, caching strains and infectivity.
        // If total infectiousness exceeds unity, we will normalize all strains down accordingly.
        for (auto infection : infections)
        {
            release_assert( infection );
            tmp_infectiousness = infection->GetInfectiousness();
            infectiousness += tmp_infectiousness;

            if ( tmp_infectiousness > 0 )
            {
                infection->GetInfectiousStrainID(&tmp_strainIDs);
                infectivity_by_strain[tmp_strainIDs] += tmp_infectiousness;
            }
        }

        // Effects of transmission-reducing immunity.  N.B. interventions on vector success are not here, since they depend on vector-population-specific behavior
        release_assert( susceptibility );
        release_assert( interventions );
        float modtransmit = susceptibility->GetModTransmit() * interventions->GetInterventionReducedTransmit();

        // Maximum individual infectiousness is set here, capping the sum of unmodified infectivity at prob=1
        float truncate_infectious_mod = (infectiousness > 1 ) ? 1.0f/infectiousness : 1.0f;
        infectiousness *= truncate_infectious_mod * modtransmit;

        // Host weight is the product of MC weighting and relative biting
        float host_vector_weight = (float) ( GetMonteCarloWeight() * GetRelativeBitingRate() );

        // Effects from vector intervention container
        IVectorInterventionsEffects* ivie = NULL;
        if ( s_OK !=  interventions->QueryInterface(GET_IID(IVectorInterventionsEffects), (void**)&ivie) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "interventions", "IVectorInterventionsEffects", "IndividualHumanVector" );
        }

        // Loop again over infection strains, depositing (downscaled) infectivity modified by vector intervention effects etc. (indoor + outdoor)
        for (auto& infectivity : infectivity_by_strain)
        {
            const StrainIdentity *id = &(infectivity.first);

            parent->DepositFromIndividual( const_cast<StrainIdentity*>(id), host_vector_weight * infectivity.second * truncate_infectious_mod * modtransmit * ivie->GetblockIndoorVectorTransmit(), &NodeVector::human_to_vector_indoor );
            parent->DepositFromIndividual( const_cast<StrainIdentity*>(id), host_vector_weight * infectivity.second * truncate_infectious_mod * modtransmit * ivie->GetblockOutdoorVectorTransmit(), &NodeVector::human_to_vector_outdoor );
        }
    }

    Infection *IndividualHumanVector::createInfection(suids::suid _suid)
    {
        return InfectionVector::CreateInfection(this, _suid);
    }
    
    float 
    IndividualHumanVector::GetRelativeBitingRate(void) const
    {
        release_assert( vector_susceptibility );
        return vector_susceptibility->GetRelativeBitingRate();
    }
}

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
namespace Kernel {

    // IJsonSerializable Interfaces
    void IndividualHumanVector::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {

        root->BeginObject();

        root->Insert("m_total_exposure", m_total_exposure);

        // m_strain_exposure is a vector of pair of StrainIdentity and float.
        // vector is a Json array while pair is array
        // so array of array
        root->Insert("m_strain_exposure");
        root->BeginArray();
        for (auto& exposure : m_strain_exposure)
        {
            root->BeginArray();
            exposure.first.JSerialize(root, helper);
            root->Add(exposure.second);
            root->EndArray();
        }
        root->EndArray();

        root->Insert("IndividualHuman");
        IndividualHuman::JSerialize( root, helper );

        root->EndObject();
    }

    void IndividualHumanVector::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
        
        rapidjson::Document * doc = (rapidjson::Document*) root; 
        
        m_total_exposure = (*doc)["m_total_exposure"].GetDouble();
        unsigned int num_strains = (*doc)["m_strain_exposure"].Size();
        LOG_INFO_F( "num_strains=%d, m_total_exposure=%f\n",num_strains, m_total_exposure);
        for( unsigned int idx = 0; idx < num_strains; idx++ )
        {
            
            StrainIdentity strain_id;
            unsigned int iid = 0;
            strain_id.JDeserialize((IJsonObjectAdapter*) &((*doc)["m_strain_exposure"][idx][iid]),helper);
            float t_exp = (*doc)["m_strain_exposure"][idx][iid+1].GetDouble();
            m_strain_exposure.push_back( std::make_pair(strain_id, t_exp) );
            LOG_INFO_F( "num_strains_aid=%d, num_strins_gid=%d, t_exp=%f\n", strain_id.GetAntigenID(), strain_id.GetGeneticID(),t_exp);
        }

        IndividualHuman::JDeserialize((IJsonObjectAdapter*) &((*doc)["IndividualHuman"]), helper);

    }

}
#endif

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::IndividualHumanVector)
namespace Kernel
{
    template<class Archive>
    void serialize(Archive & ar, IndividualHumanVector& human, const unsigned int  file_version )
    {
        ar.template register_type<Kernel::InfectionVector>();
        ar.template register_type<Kernel::SusceptibilityVector>();
        ar.template register_type<Kernel::VectorInterventionsContainer>();
            
        // Serialize fields - N/A
        ar & human.m_strain_exposure;
        ar & human.m_total_exposure;

        // Serialize base class
        ar & boost::serialization::base_object<Kernel::IndividualHuman>(human);
    }
    template void serialize( boost::mpi::detail::mpi_datatype_oarchive&, Kernel::IndividualHumanVector&, unsigned int);
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::IndividualHumanVector&, unsigned int);
    template void serialize( boost::mpi::packed_skeleton_oarchive&, Kernel::IndividualHumanVector&, unsigned int);
    template void serialize( boost::mpi::packed_iarchive&, Kernel::IndividualHumanVector&, unsigned int);
    template void serialize( boost::mpi::packed_oarchive&, Kernel::IndividualHumanVector&, unsigned int);
    template void serialize( boost::mpi::detail::content_oarchive&, Kernel::IndividualHumanVector&, unsigned int);
    template void serialize( boost::archive::binary_oarchive&, Kernel::IndividualHumanVector&, unsigned int);
    template void serialize( boost::archive::binary_iarchive&, Kernel::IndividualHumanVector&, unsigned int);
}
#endif

