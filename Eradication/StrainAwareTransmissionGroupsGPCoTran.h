
#pragma once

#include "StrainAwareTransmissionGroupsGP.h"

using namespace std;

namespace Kernel
{
    // In CoTransmission, the number of "strains" ends up being the number of people or vectors.
    // This means the standard algorithm can take a long time since every human could be looping
    // over every vector every time step.  If we are not removing vectors after they bite someone,
    // then the list of vectors/humans stays constant per time step.  Hence, we can develop the
    // total contagion list once and use std::lower_bound() to find each human/vector.
    class ContagionPopulationSubstrainGPCoTran : public ContagionPopulationSubstrainGP
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        ContagionPopulationSubstrainGPCoTran( RANDOMBASE* prng,
                                              int _antigenId,
                                              const GeneticProbability& rQuantity,
                                              const SubstrainMapTemplate<GeneticProbability>& _substrainDistribution );
        virtual ~ContagionPopulationSubstrainGPCoTran();

        // IContagionPopulation
        virtual bool ResolveInfectingStrain( IStrainIdentity* strainId ) const override;

    protected:
        float m_TotalRawContagion;
        std::vector<uint32_t> m_SubStrainIds;
        std::vector<float>    m_ContagionBySubStrainId;
    };


    // We need EndUpdate to clear our cached m_pContagionPop object and
    // exposeCandidate() to create one if it is null.  This allows us to
    // have one for each time step.
    class StrainAwareTransmissionGroupsGPCoTran : public StrainAwareTransmissionGroupsGP
    {
    public:
        StrainAwareTransmissionGroupsGPCoTran( RANDOMBASE* prng );
        virtual ~StrainAwareTransmissionGroupsGPCoTran();

        // ITransmissionGroups
        virtual void EndUpdate(float infectivityMultiplier = 1.0f, float infectivityAddition = 0.0f ) override;
        
    protected:
        virtual void exposeCandidate( IInfectable* candidate,
                                      int iAntigen,
                                      const GeneticProbability& forceOfInfection,
                                      const SubstrainMapTemplate<GeneticProbability>& substrainMap,
                                      float dt,
                                      TransmissionRoute::Enum txRoute ) override;

        ContagionPopulationSubstrainGPCoTran* m_pContagionPop;
    };

}
