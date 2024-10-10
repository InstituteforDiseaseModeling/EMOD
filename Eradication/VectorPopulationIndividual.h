
#pragma once

#include "VectorPopulation.h"

namespace Kernel
{
    struct IVectorCohortIndividual;
    class VectorCohortIndividual;

    class VectorPopulationIndividual : public VectorPopulation
    {

        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorPopulationIndividual *CreatePopulation( INodeContext *context,
                                                             int speciesIndex,
                                                             uint32_t _adult,
                                                             uint32_t mosquito_weight );
        virtual ~VectorPopulationIndividual() override;

        // IVectorPopulation
        virtual void Vector_Migration( float dt, VectorCohortVector_t* pMigratingQueue, bool migrate_males_only ) override;
        virtual void AddImmigratingVector( IVectorCohort* pvc ) override;

        // IInfectable
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route ) override;

        // IVectorPopulationReporting
        virtual uint32_t getInfectedCount(   IStrainIdentity* pStrain ) const override;
        virtual uint32_t getInfectiousCount( IStrainIdentity* pStrain ) const override;
        virtual std::vector<uint32_t> GetNewlyInfectedVectorIds()   const override;
        virtual std::vector<uint32_t> GetInfectiousVectorIds()      const override;
        virtual std::map<uint32_t, uint32_t> getNumInfectiousByCohort() const override;

    protected:
        VectorPopulationIndividual(uint32_t mosquito_weight=1); // default needed for serialization

        virtual VectorCohortIndividual *CreateAdultCohort( uint32_t vectorID,
                                                           VectorStateEnum::Enum state,
                                                           float age,
                                                           float progress,
                                                           float microsporidiaDuration,
                                                           uint32_t initial_population,
                                                           const VectorGenome& rGenome,
                                                           int speciesIndex );

        virtual void AddInitialFemaleCohort( const VectorGenome& rGenomeFemale,
                                             const VectorGenome& rGenomeMate,
                                             uint32_t num ) override;

        virtual uint32_t ProcessFeedingCycle( float dt, IVectorCohort* cohort ) override;
        void ExposeCohortList( const IContagionPopulation* cp,
                               VectorCohortVector_t& list,
                               const GeneticProbability& success_prob_gp,
                               const GeneticProbability& infection_prob_gp,
                               bool isIndoors );
        virtual void AcquireNewInfection( uint32_t vectorID,
                                          IVectorCohortIndividual* pVCI,
                                          const StrainIdentity& rStrain,
                                          bool isIndoors );
        uint32_t CalculateOvipositionTime();
        void RandomlySetOvipositionTimer( VectorCohortIndividual* pvci );

        virtual void Update_Infectious_Queue( float dt ) override;
        virtual void Update_Infected_Queue( float dt ) override;
        virtual void Update_Adult_Queue( float dt ) override;

        virtual void AddAdultsAndMate( IVectorCohort* pFemaleCohort,
                                       VectorCohortCollectionAbstract& rQueue,
                                       bool isNewAdult ) override;
        virtual void AddAdultCohort( IVectorCohort* pFemaleCohort,
                                     const VectorGenome& rMaleGenome,
                                     uint32_t pop,
                                     VectorCohortCollectionAbstract& rQueue,
                                     bool isNewAdult ) override;
        virtual void AddReleasedCohort( VectorStateEnum::Enum state,
                                        const VectorGenome& rFemaleGenome,
                                        const VectorGenome& rMaleGenome,
                                        uint32_t pop ) override;

        virtual void AdjustForCumulativeProbability( FeedingProbabilities& rFeedProbs );

        bool DidDie(  IVectorCohort* cohort,
                     float probDies,
                     float outcome,
                     bool enableMortality,
                     uint32_t& rCounter,
                     const char* msg,
                     const char* name );
        bool DidFeed( IVectorCohort* cohort,
                      float probFeed,
                      float outcome,
                      uint32_t& rCounter,
                      const char* msg,
                      const char* name );
        bool DiedLayingEggs( IVectorCohort* cohort, const FeedingProbabilities& rFeedProbs );

        virtual void StartGestating( uint32_t numFed, IVectorCohort* cohort ) override;

        virtual void UpdateGestatingCount( const IVectorCohort* pvc ) override;

        virtual IndoorOutdoorResults ProcessIndoorOutdoorFeeding( const char* pIndoorOutdoorName,
                                                                  const IndoorOutdoorProbabilities& rProbs,
                                                                  float probHumanFeed,
                                                                  IVectorCohort* cohort,
                                                                  TransmissionRoute::Enum routeVectorToHuman,
                                                                  VectorCohortVector_t& rExposedQueue );

        uint32_t CountStrains( VectorStateEnum::Enum state,
                               const VectorCohortCollectionAbstract& queue,
                               IStrainIdentity* pStrain ) const;

        uint32_t m_mosquito_weight;
        float m_average_oviposition_killing;

        // Temporary containers to store exposed vectors before exposing them to infectious strains
        // These could be moved back to the base class, VectorPopulation, if we figure out a sensible way 
        // to deal with minimal strain tracking in the cohort model
        VectorCohortVector_t IndoorExposedQueues;
        VectorCohortVector_t OutdoorExposedQueues;

        IVectorCohortIndividual * current_vci; // added this since we have it, then call a function, and function re-qi's for it, which is unnecessary.

        DECLARE_SERIALIZABLE(VectorPopulationIndividual);
    };
}
