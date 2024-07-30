
#pragma once

#include "VectorPopulationIndividual.h"
#include "MalariaGeneticsContexts.h"

namespace Kernel
{
    struct INodeMalariaGenetics;

    class VectorPopulationIndividualMalariaGenetics : public VectorPopulationIndividual
                                                    , public IVectorPopulationReportingMalariaGenetics
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        static VectorPopulationIndividualMalariaGenetics *CreatePopulation( INodeContext *context,
                                                                          int speciesIndex,
                                                                          uint32_t _adult,
                                                                          uint32_t mosquito_weight );
        virtual ~VectorPopulationIndividualMalariaGenetics() override;

        virtual void SetContextTo( INodeContext *context ) override;
        virtual void UpdateVectorPopulation( float dt ) override;

        // IVectorPopulationReportingMalariaGenetics
        virtual void RegisterCounter( IVectorCounter* pCounter ) override;
        virtual void ExtractOtherVectorStats( OtherVectorStats& rOVS ) const override;

    protected:
        VectorPopulationIndividualMalariaGenetics( uint32_t mosquito_weight = 1 ); // default needed for serialization

        virtual VectorCohortIndividual *CreateAdultCohort( uint32_t vectorID,
                                                           VectorStateEnum::Enum state,
                                                           float age,
                                                           float progress,
                                                           float microsporidiaDuration,
                                                           uint32_t initial_population,
                                                           const VectorGenome& rGenome,
                                                           int speciesIndex ) override;

        virtual void Update_Adult_Queue( float dt ) override;
        virtual IndoorOutdoorResults ProcessIndoorOutdoorFeeding( const char* pIndoorOutdoorName,
                                                                  const IndoorOutdoorProbabilities& rProbs,
                                                                  float probHumanFeed,
                                                                  IVectorCohort* cohort,
                                                                  TransmissionRoute::Enum routeVectorToHuman,
                                                                  VectorCohortVector_t& rExposedQueues ) override;
        virtual void queueIncrementNumInfs( IVectorCohort* cohort ) override;

        IndoorOutdoorProbabilities GetProbabilitiesForPerson( IVectorCohort* pCohort,
                                                              TransmissionRoute::Enum routeVectorToHuman,
                                                              IVectorInterventionsEffects* pIVIE,
                                                              const IndoorOutdoorProbabilities& rProbs );

        INodeMalariaGenetics *m_pNodeGenetics;
        uint32_t m_NumBitesAdult;
        uint32_t m_NumBitesInfected;
        uint32_t m_NumBitesInfectious;
        std::vector<IVectorCounter*> m_VectorCounters;

        DECLARE_SERIALIZABLE( VectorPopulationIndividualMalariaGenetics );
    };
}
