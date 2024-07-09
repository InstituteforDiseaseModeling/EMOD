
#pragma once

#include "StrainAwareTransmissionGroups.h"
#include "GeneticProbability.h"
#include "IContagionPopulationGP.h"

using namespace std;

namespace Kernel
{
    // This version of StrainAwareTransmissionGroups is needed to support
    // Insecticide Resistance where we care about the genetics of the vector
    // depositing and being exposed.  The GeneticProbability object can
    // contain different probabilities for different vector genomes.
    class StrainAwareTransmissionGroupsGP : public StrainAwareTransmissionGroupsTemplate<GeneticProbability>
    {
    public:
        StrainAwareTransmissionGroupsGP( RANDOMBASE* prng );
        virtual ~StrainAwareTransmissionGroupsGP();

        // ITransmissionGroups - Don't allow these methods
        virtual void DepositContagion( const IStrainIdentity& strain,
                                       float amount,
                                       TransmissionGroupMembership_t transmissionGroupMembership ) override;
        virtual float GetContagionByProperty( const IPKeyValue& property_value ) override;
        virtual float GetTotalContagion( void ) override;
        virtual float GetTotalContagionForGroup( TransmissionGroupMembership_t group ) override;

        // Methods used by NodeVector in place the methods that are part of ITransmissionGroups
        void DepositContagionGP( const IStrainIdentity& strain,
                                 const GeneticProbability& rAmount,
                                 TransmissionGroupMembership_t transmissionGroupMembership);
        GeneticProbability GetTotalContagionGP();
        
    protected:
        virtual void exposeCandidate( IInfectable* candidate,
                                      int iAntigen,
                                      const GeneticProbability& forceOfInfection,
                                      const SubstrainMapTemplate<GeneticProbability>& substrainMap,
                                      float dt,
                                      TransmissionRoute::Enum txRoute ) override;
        virtual bool isGreaterThanZero( const GeneticProbability& forceOfInfection ) const override;
    };

    class ContagionPopulationSubstrainGP : public IContagionPopulation
                                         , public IContagionPopulationGP
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        ContagionPopulationSubstrainGP( RANDOMBASE* prng,
                                        int _antigenId,
                                        const GeneticProbability& rQuantity,
                                        const SubstrainMapTemplate<GeneticProbability>& _substrainDistribution );
        virtual ~ContagionPopulationSubstrainGP();

        // IContagionPopulation
        virtual int GetAntigenID(void) const override;
        virtual float GetTotalContagion() const override;
        virtual bool ResolveInfectingStrain( IStrainIdentity* strainId ) const override;

        // IContagionPopulationGP
        virtual const GeneticProbability& GetTotalContagionGP() const override;

    protected:
        RANDOMBASE* m_pRNG;
        int m_AntigenID;
        GeneticProbability m_Quantity;
        const SubstrainMapTemplate<GeneticProbability>& m_rSubstrainDistribution;
    };

}
