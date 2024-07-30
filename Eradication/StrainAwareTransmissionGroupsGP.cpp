
#include "stdafx.h"

#include "StrainAwareTransmissionGroupsGP.h"
#include "Exceptions.h"
#include "RANDOM.h"
#include "Log.h"
#include "Debug.h"
#include "IStrainIdentity.h"

SETUP_LOGGING( "StrainAwareTransmissionGroupsGP" )

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- StrainAwareTransmissionGroupsGP
    // ------------------------------------------------------------------------

    StrainAwareTransmissionGroupsGP::StrainAwareTransmissionGroupsGP( RANDOMBASE* prng )
        : StrainAwareTransmissionGroupsTemplate( prng )
    {
    }

    StrainAwareTransmissionGroupsGP::~StrainAwareTransmissionGroupsGP()
    {
    }

    void StrainAwareTransmissionGroupsGP::DepositContagion( const IStrainIdentity& strain,
                                                            float amount,
                                                            TransmissionGroupMembership_t transmissionGroupMembership )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should be calling DepositContagionGP" );
    }

    float StrainAwareTransmissionGroupsGP::GetTotalContagion( void )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should be calling GetTotalContagionGP" );
    }

    float StrainAwareTransmissionGroupsGP::GetContagionByProperty( const IPKeyValue& property_value )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Not allowed/implemented" );
    }

    float StrainAwareTransmissionGroupsGP::GetTotalContagionForGroup( TransmissionGroupMembership_t membership )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Not allowed/implemented" );
    }

    void StrainAwareTransmissionGroupsGP::DepositContagionGP( const IStrainIdentity& strain,
                                                              const GeneticProbability& amount,
                                                              TransmissionGroupMembership_t transmissionGroupMembership )
    {
        depositContagionInner( strain, amount, transmissionGroupMembership );
    }

    GeneticProbability StrainAwareTransmissionGroupsGP::GetTotalContagionGP()
    {
        return totalContagion;
    }

    void StrainAwareTransmissionGroupsGP::exposeCandidate( IInfectable* candidate, 
                                                           int iAntigen,
                                                           const GeneticProbability& forceOfInfection,
                                                           const SubstrainMapTemplate<GeneticProbability>& substrainMap,
                                                           float dt,
                                                           TransmissionRoute::Enum txRoute )
    {

        ContagionPopulationSubstrainGP contagionPopulation( pRNG, iAntigen, forceOfInfection, substrainMap );
        candidate->Expose( (IContagionPopulation*)&contagionPopulation, dt, txRoute );
    }

    bool StrainAwareTransmissionGroupsGP::isGreaterThanZero( const GeneticProbability& forceOfInfection ) const
    {
        return (forceOfInfection.GetSum() > 0.0f);
    }

    // ------------------------------------------------------------------------
    // --- ContagionPopulationSubstrain
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(ContagionPopulationSubstrainGP)
        HANDLE_INTERFACE(IContagionPopulation)
        HANDLE_INTERFACE(IContagionPopulationGP)
    END_QUERY_INTERFACE_BODY(ContagionPopulationSubstrainGP)

    ContagionPopulationSubstrainGP::ContagionPopulationSubstrainGP( RANDOMBASE* prng,
                                                                    int antigenID,
                                                                    const GeneticProbability& quantity,
                                                                    const SubstrainMapTemplate<GeneticProbability>& rSubstrainDistribution )
        : m_pRNG( prng )
        , m_AntigenID( antigenID )
        , m_Quantity( quantity )
        , m_rSubstrainDistribution(rSubstrainDistribution)
    {
    }

    ContagionPopulationSubstrainGP::~ContagionPopulationSubstrainGP()
    {
    }

    int ContagionPopulationSubstrainGP::GetAntigenID() const
    {
        return m_AntigenID;
    }

    float ContagionPopulationSubstrainGP::GetTotalContagion() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Call GetTotalContagionGP" );
    }

    bool ContagionPopulationSubstrainGP::ResolveInfectingStrain( IStrainIdentity* strainId ) const
    {
        LOG_VALID_F( "%s\n", __FUNCTION__ );

        strainId->SetAntigenID( GetAntigenID() );
        strainId->SetGeneticID( 0 );

        float totalRawContagion = 0.0f;
        for (auto& entry : m_rSubstrainDistribution)
        {
            totalRawContagion += entry.second.GetSum();
        }

        if (totalRawContagion == 0.0f)
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
        float target = totalRawContagion * rand;
        float contagionSeen = 0.0f;
        int substrainId = 0;

        for (auto& entry : m_rSubstrainDistribution)
        {
            float contagion = entry.second.GetSum();
            if (contagion > 0.0f)
            {
                substrainId = entry.first;
                contagionSeen += contagion;
                if (contagionSeen >= target)
                {
                    LOG_DEBUG_F( "Selected strain id %d\n", substrainId );
                    strainId->SetGeneticID(substrainId); // ????
                    return true;
                }
            }
        }

        // commenting out because Malaria CoTransmission can have this happen
        //LOG_WARN_F( "Ran off the end of the distribution (rounding error?). Using last valid sub-strain we saw: %d\n", substrainId );
        strainId->SetGeneticID(substrainId);

        return false;
    }

    const GeneticProbability& ContagionPopulationSubstrainGP::GetTotalContagionGP() const
    {
        return m_Quantity;
    }
}