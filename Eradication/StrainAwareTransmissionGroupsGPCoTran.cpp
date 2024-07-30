
#include "stdafx.h"

#include "StrainAwareTransmissionGroupsGPCoTran.h"
#include "Exceptions.h"
#include "RANDOM.h"
#include "Log.h"
#include "Debug.h"
#include "IStrainIdentity.h"

SETUP_LOGGING( "StrainAwareTransmissionGroupsGPCoTran" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- StrainAwareTransmissionGroupsGPCoTran
    // ------------------------------------------------------------------------

    StrainAwareTransmissionGroupsGPCoTran::StrainAwareTransmissionGroupsGPCoTran( RANDOMBASE* prng )
        : StrainAwareTransmissionGroupsGP( prng )
        , m_pContagionPop( nullptr )
    {
    }

    StrainAwareTransmissionGroupsGPCoTran::~StrainAwareTransmissionGroupsGPCoTran()
    {
        delete m_pContagionPop;
        m_pContagionPop = nullptr;
    }

    void StrainAwareTransmissionGroupsGPCoTran::EndUpdate( float infectivityMultiplier,
                                                           float infectivityAddition )
    {
        StrainAwareTransmissionGroupsGP::EndUpdate( infectivityMultiplier, infectivityAddition );
        delete m_pContagionPop;
        m_pContagionPop = nullptr;
    }

    void StrainAwareTransmissionGroupsGPCoTran::exposeCandidate( IInfectable* candidate, 
                                                                 int iAntigen,
                                                                 const GeneticProbability& forceOfInfection,
                                                                 const SubstrainMapTemplate<GeneticProbability>& substrainMap,
                                                                 float dt,
                                                                 TransmissionRoute::Enum txRoute )
    {
        if( m_pContagionPop == nullptr )
        {
            m_pContagionPop = new ContagionPopulationSubstrainGPCoTran( pRNG, iAntigen, forceOfInfection, substrainMap );
        }
        candidate->Expose( (IContagionPopulation*)m_pContagionPop, dt, txRoute );
    }

    // ------------------------------------------------------------------------
    // --- ContagionPopulationSubstrain
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_DERIVED(ContagionPopulationSubstrainGPCoTran,ContagionPopulationSubstrainGP)
    END_QUERY_INTERFACE_DERIVED(ContagionPopulationSubstrainGPCoTran,ContagionPopulationSubstrainGP)

    ContagionPopulationSubstrainGPCoTran::ContagionPopulationSubstrainGPCoTran( RANDOMBASE* prng,
                                                                                int antigenID,
                                                                                const GeneticProbability& quantity,
                                                                                const SubstrainMapTemplate<GeneticProbability>& rSubstrainDistribution )
        : ContagionPopulationSubstrainGP( prng, antigenID, quantity, rSubstrainDistribution )
        , m_TotalRawContagion( 0.0 )
        , m_SubStrainIds()
        , m_ContagionBySubStrainId()
    {
        m_SubStrainIds.reserve( rSubstrainDistribution.size() );
        m_ContagionBySubStrainId.reserve( rSubstrainDistribution.size() );
        for (auto& entry : rSubstrainDistribution)
        {
            float contagion = entry.second.GetSum();
            m_TotalRawContagion += contagion;
            m_SubStrainIds.push_back( entry.first );
            m_ContagionBySubStrainId.push_back( m_TotalRawContagion );
        }
    }

    ContagionPopulationSubstrainGPCoTran::~ContagionPopulationSubstrainGPCoTran()
    {
    }

    bool ContagionPopulationSubstrainGPCoTran::ResolveInfectingStrain( IStrainIdentity* strainId ) const
    {
        LOG_VALID_F( "%s\n", __FUNCTION__ );

        strainId->SetAntigenID( GetAntigenID() );
        strainId->SetGeneticID( 0 );

        if( m_TotalRawContagion == 0.0f )
        {
            // ---------------------------------------------------------------------------
            // --- Because Malaria CoTransmission uses ClearStrain() to remove "strains"
            // --- (i.e. vectors and humans).  The actual "raw" contagion can go away.
            // --- This implies that all of the bites have been distributed.
            // ---------------------------------------------------------------------------

            // commenting out because Malaria CoTransmission can have this happen
            //LOG_WARN_F( "Found no raw contagion for antigen=%d (%f total contagion)\n", antigenId, float(contagionQuantity));

            return false;
        }

        float rand = m_pRNG->e();
        float target = m_TotalRawContagion * rand;

        vector<float>::const_iterator it = std::lower_bound( m_ContagionBySubStrainId.begin(),
                                                             m_ContagionBySubStrainId.end(),
                                                             target );
        if( it != m_ContagionBySubStrainId.end() )
        {
            int index = it - m_ContagionBySubStrainId.begin();

            LOG_DEBUG_F( "Selected strain id %d\n", m_SubStrainIds[ index ] );
            strainId->SetGeneticID( m_SubStrainIds[ index ] );
            return true;
        }
        else
        {
            // commenting out because Malaria CoTransmission can have this happen
            strainId->SetGeneticID( 0 );
            return false;
        }
    }
}