/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <list>
#include <map>

#include "Common.h"
#include "IContagionPopulation.h"
#include "IInfectable.h"
#include "SimulationEnums.h"

#include "Vector.h"
#include "VectorEnums.h"
#include "IVectorCohort.h"
#include "VectorContexts.h"
#include "VectorHabitat.h"
#include "VectorMatingStructure.h"
#include "VectorProbabilities.h"

#include "ISerializable.h"
#include "IVectorPopulation.h"

namespace Kernel
{
    class IVectorCohortWithHabitat;
    class SimulationConfig ;
    class VectorSpeciesParameters;
    struct ITransmissionGroups;
    struct IMigrationInfo;

    class IndividualHumanVector;
    class VectorCohortWithHabitat;
    class RANDOMBASE;

    class VectorPopulation : public IVectorPopulation, public IInfectable, public IVectorPopulationReporting
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorPopulation *CreatePopulation( INodeContext *context, const std::string& species, uint32_t adults, uint32_t infectious );
        virtual ~VectorPopulation();

        // ----------------------
        // --- IVectorPopulation
        // ----------------------
        virtual void SetContextTo(INodeContext *context) override;
        virtual void SetupIntranodeTransmission(ITransmissionGroups *txIndoor, ITransmissionGroups* txOutdoor) override;
        virtual void SetupLarvalHabitat( INodeContext *context ) override;
        virtual void SetVectorMortality( bool mortality ) override { m_VectorMortality = mortality; }

        // The function that NodeVector calls into once per species per timestep
        virtual void UpdateVectorPopulation(float dt) override;

        // For NodeVector to calculate # of migrating vectors (processEmigratingVectors) and put them in new node (processImmigratingVector)
        virtual void Vector_Migration( float dt, IMigrationInfo* pMigInfo, VectorCohortVector_t* pMigratingQueue ) override;
        virtual void AddImmigratingVector( IVectorCohort* pvc ) override;

        // Supports MosquitoRelease intervention
        virtual void AddVectors( const VectorMatingStructure& _vector_genetics, uint32_t releasedNumber ) override;

        // ---------------
        // --- IInfectable
        // ---------------
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route ) override;
        virtual const infection_list_t& GetInfections() const override;
        virtual float GetInterventionReducedAcquire() const override;

        // -------------------------------
        // --- IVectorPopulationReporting
        // -------------------------------
        virtual float  GetEIRByPool(VectorPoolIdEnum::Enum pool_id)     const override;
        virtual float  GetHBRByPool(VectorPoolIdEnum::Enum pool_id)     const override;
        virtual uint32_t getAdultCount()                                const override;
        virtual uint32_t getInfectedCount(   IStrainIdentity* pStrain ) const override;
        virtual uint32_t getInfectiousCount( IStrainIdentity* pStrain ) const override;
        virtual uint32_t getMaleCount()                                 const override;
        virtual uint32_t getNewEggsCount()                              const override;
        virtual uint32_t getNewAdults()                                 const override;
        virtual uint32_t getNumDiedBeforeFeeding()                      const override;
        virtual uint32_t getNumDiedDuringFeedingIndoor()                const override;
        virtual uint32_t getNumDiedDuringFeedingOutdoor()               const override;
        virtual double  getInfectivity()                                const override;
        virtual const std::string& get_SpeciesID()                      const override;
        virtual const VectorHabitatList_t& GetHabitats()                const override;
        virtual std::vector<uint64_t> GetNewlyInfectedVectorIds()       const override;
        virtual std::vector<uint64_t> GetInfectiousVectorIds()          const override;

    protected:
        VectorPopulation();
        void Initialize( INodeContext *context, const std::string& species_name, uint32_t adults, uint32_t infectious );
        virtual void InitializeVectorQueues(unsigned int adults, unsigned int _infectious);

        void UpdateLocalAdultMortalityProbability( float dt );
        float GetLocalAdultMortalityProbability( float dt, IVectorCohort* pvc, VectorWolbachia::Enum wolb = VectorWolbachia::WOLBACHIA_FREE ) const;
        void UpdateAge( IVectorCohort* pvc, float dt );

        const SimulationConfig        *params()  const ;
        const VectorSpeciesParameters *species() const { return m_species_params; }
        VectorProbabilities           *probs()         { return m_probabilities; }

        // This is the senescence equation for adult mortality in Ae aegypti as measured in lab experiments (Styer et al 2007)
        // Here this becomes an additional aging mortality superimposed on the specified realworld mosquito mortality in the field
        // inline static float mortalityFromAge(float age) { return (0.006f * exp(0.2f * age) / (1.0f + (0.006f * 1.5f / 0.2f * (exp(0.2f * age) - 1.0f)))); }
        // You would think that the compile would optimize the above code, but it appears that it does not, so we hand code the following:
        inline static float mortalityFromAge(float age) { float e = exp(0.2f*age); return (0.006f * e / (1.0f + (0.0449999981f * (e - 1)))); }

        // High temperatures and low humidity can decrease vector life expectancy
        // Craig, M. H., R. W. Snow, et al. (1999). "A climate-based distribution model of malaria transmission in sub-Saharan Africa." Parasitol Today 15(3): 105-111.
        // Here we adapt the formula of Martens to get a rate which is better for the mathematical formalism in this class
        inline static float dryHeatMortality(float temperature) { return .001f * exp(temperature - 32.0f); }

        // Calculations of life-cycle probabilities
        virtual void Update_Lifecycle_Probabilities(float dt);

        // Updating the various life-cycle stages
        virtual void Update_Infectious_Queue( float dt );
        virtual void Update_Infected_Queue  ( float dt );
        virtual void Update_Adult_Queue     ( float dt );
        virtual void Update_Male_Queue      ( float dt );
        virtual void Update_Immature_Queue  ( float dt );
        virtual void Update_Larval_Queue    ( float dt );
        virtual void Update_Egg_Laying      ( float dt );
        virtual void Update_Egg_Hatching    ( float dt );

        virtual void AddAdultsFromMating( const VectorGeneticIndex_t& rVgiMale,
                                          const VectorGeneticIndex_t& rVgiFemale,
                                          uint32_t pop );
        virtual void AddVectors_Adults( const VectorMatingStructure& _vector_genetics, uint32_t releasedNumber );

        // Factorize the feeding cycle, which is common to all adult vectors
        virtual uint32_t ProcessFeedingCycle( float dt, IVectorCohort* cohort );
        float GetFeedingCycleDuration() const;

        // Calculation of mated females based on gender_mating characteristics of male/female populations
        virtual void ApplyMatingGenetics( IVectorCohort* cohort, const VectorMatingStructure& male_vector_genetics );

        // Helper functions for egg calculations
        void CreateEggCohortOfType(        IVectorHabitat*, uint32_t, const VectorMatingStructure& ); // don't need to copy
        void CreateEggCohortAlleleSorting( IVectorHabitat*, uint32_t, VectorMatingStructure ); //need to copy VectorMatingStructure
        void CreateEggCohortHEGSorting(    IVectorHabitat*, uint32_t, VectorMatingStructure ); //need to copy VectorMatingStructure

        // Seek a compatible (same gender mating type) queue in specified list (e.g. AdultQueues, InfectiousQueues) and increase its population.
        virtual void MergeProgressedCohortIntoCompatibleQueue( VectorCohortVector_t &queues, IVectorCohort* pvc, float progressThisTimestep );

        // Helpers to access information from VectorHabitat to return information about larva
        float GetLarvalDevelopmentProgress (float dt, IVectorCohortWithHabitat* larva) const;
        float GetLarvalMortalityProbability(float dt, IVectorCohortWithHabitat* larva) const;
        float GetRelativeSurvivalWeight(VectorHabitat* habitat) const;

        // VectorPopulation accounting helper function
        virtual void queueIncrementTotalPopulation( IVectorCohort* cohort );

        static std::vector<uint32_t> GetRandomIndexes( RANDOMBASE* pRNG, uint32_t N );

        void Vector_Migration_Queue( const std::vector<uint32_t>& rRandomIndexes,
                                     const std::vector<suids::suid>& rReachableNodes,
                                     const std::vector<MigrationType::Enum>& rMigrationTypes,
                                     const std::vector<float>& rRates,
                                     VectorCohortVector_t* pMigratingQueue,
                                     VectorCohortVector_t& rQueue );

        struct FeedingProbabilities
        {
            float die_without_attempting_to_feed;
            float die_before_human_feeding;
            float successful_feed_animal;
            float successful_feed_artifical_diet;
            float successful_feed_attempt_indoor;
            float successful_feed_attempt_outdoor;
            float die_indoor;
            float successful_feed_artifical_diet_indoor;
            float successful_feed_human_indoor;
            float die_outdoor;
            float successful_feed_human_outdoor;

            FeedingProbabilities()
                : die_without_attempting_to_feed(0.0f)
                , die_before_human_feeding( 0.0f )
                , successful_feed_animal( 0.0f )
                , successful_feed_artifical_diet( 0.0f )
                , successful_feed_attempt_indoor( 0.0f )
                , successful_feed_attempt_outdoor( 0.0f )
                , die_indoor( 0.0f )
                , successful_feed_artifical_diet_indoor( 0.0f )
                , successful_feed_human_indoor( 0.0f )
                , die_outdoor( 0.0f )
                , successful_feed_human_outdoor( 0.0f )
            {
            }
        };

        float CalculateEggBatchSize( IVectorCohort* cohort );
        virtual void GenerateEggs( uint32_t numFeedHuman, uint32_t numFeedAD, uint32_t numFeedAnimal, IVectorCohort* cohort );
        void AdjustEggsForDeath( IVectorCohort* cohort, uint32_t numDied );
        void AddEggsToLayingQueue( IVectorCohort* cohort, uint32_t num_eggs );

        uint32_t CalculatePortionInProbability( bool isForDeath, uint32_t& rRemainingPop, float prob );
        float AdjustForConditionalProbability( float& rCumulative, float probability );

        virtual void AdjustForFeedingRate( float dt, float p_local_mortality, FeedingProbabilities& rFeedProbs );
        virtual void AdjustForCumulativeProbability( FeedingProbabilities& rFeedProbs );
        FeedingProbabilities CalculateFeedingProbabilities( float dt, IVectorCohort* cohort );

        virtual void VectorToHumanDeposit( const IStrainIdentity& strain,
                                           uint32_t attemptFeed,
                                           TransmissionRoute::Enum route );

        uint32_t VectorToHumanTransmission( IVectorCohort* cohort,
                                            uint32_t attemptFeed,
                                            TransmissionRoute::Enum route );

        uint32_t CalculateHumanToVectorInfection( TransmissionRoute::Enum route,
                                                  IVectorCohort* cohort,
                                                  float probSuccessfulFeed,
                                                  uint32_t numHumanFeed );

        // List of habitats for this species
        VectorHabitatList_t* m_larval_habitats; // "shared" pointer, NodeVector owns this memory
        std::map<VectorHabitatType::Enum, float> m_larval_capacities;

        uint32_t neweggs; 
        uint32_t adult;      // female population
        uint32_t infected;
        uint32_t infectious;
        uint32_t males;

        uint32_t new_adults;
        uint32_t dead_mosquitoes_before;
        uint32_t dead_mosquitoes_indoor;
        uint32_t dead_mosquitoes_outdoor;

        // local variations on base rates
        float dryheatmortality;
        float infectiouscorrection;
        float infected_progress_this_timestep;

        // intermediate counters
        float indoorinfectiousbites;
        float outdoorinfectiousbites;
        float indoorbites;
        float outdoorbites;

        // output
        float infectivity;
        float infectivity_indoor;
        float infectivity_outdoor;

        // reporting (first is INDOOR_VECTOR_POOL, second is OUTDOOR_VECTOR_POOL)
        std::pair<float, float>  m_EIR_by_pool;
        std::pair<float, float>  m_HBR_by_pool;

        // counters to track population stats (Wolbachia type, sterility, etc...)
        typedef std::map<VectorGeneticIndex_t,int> VectorMatingMap_t;
        VectorMatingMap_t gender_mating_eggs;
        VectorMatingMap_t gender_mating_males;
        // These three Maps are for reporting purposes only
        VectorMatingMap_t vector_genetics_adults;
        VectorMatingMap_t vector_genetics_infected;
        VectorMatingMap_t vector_genetics_infectious;

        std::string species_ID;

        VectorCohortVector_t EggQueues;
        VectorCohortVector_t LarvaQueues;
        VectorCohortVector_t ImmatureQueues;
        VectorCohortVector_t AdultQueues;
        VectorCohortVector_t InfectedQueues;
        VectorCohortVector_t InfectiousQueues;
        VectorCohortVector_t MaleQueues;

        INodeContext                  *m_context;
        const VectorSpeciesParameters *m_species_params;
        VectorProbabilities           *m_probabilities;
        ITransmissionGroups           *m_txIndoor;
        ITransmissionGroups           *m_txOutdoor;

        bool m_VectorMortality;

        std::vector<std::vector<float>> m_LocalMortalityProbabilityTable;
        float m_DefaultLocalMortalityProbability;

        static std::vector<float> m_MortalityTable;

        DECLARE_SERIALIZABLE(VectorPopulation);
    };

    // clorton TODO - move these into IArchive
    void serialize(IArchive&, std::pair<float, float>&);
    void serialize(IArchive&, std::map<uint32_t, int>&);
}
