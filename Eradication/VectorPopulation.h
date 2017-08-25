/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <list>
#include <map>

#include "Common.h"
#include "IContagionPopulation.h"
#include "IInfectable.h"

#include "Vector.h"
#include "VectorEnums.h"
#include "VectorCohort.h"
#include "VectorContexts.h"
#include "VectorHabitat.h"
#include "VectorMatingStructure.h"
#include "VectorProbabilities.h"

#include "ISerializable.h"

namespace Kernel
{
    class IVectorCohortWithHabitat;
    class SimulationConfig ;
    class VectorSpeciesParameters;

    struct IVectorPopulation : ISerializable
    {
    };

    class IndividualHumanVector;
    class VectorCohortWithHabitat;

    class VectorPopulation : public IVectorPopulation, public IInfectable, public IVectorPopulationReporting
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorPopulation *CreatePopulation(INodeContext *context, std::string species = "gambiae", unsigned int adults = DEFAULT_VECTOR_POPULATION_SIZE, unsigned int infectious = 0);
        virtual ~VectorPopulation();

        virtual void SetContextTo(INodeContext *context);
        virtual void SetupIntranodeTransmission(ITransmissionGroups *transmissionGroups);
        virtual void SetupLarvalHabitat( INodeContext *context );

        // The function that NodeVector calls into once per species per timestep
        virtual void UpdateVectorPopulation(float dt);

        // For NodeVector to calculate # of migrating vectors (processEmigratingVectors) and put them in new node (processImmigratingVector)
        virtual void Vector_Migration( IMigrationInfo* pMigInfo, VectorCohortVector_t* pMigratingQueue );
        virtual uint64_t Vector_Migration(float = 0, VectorCohortVector_t* = nullptr);
        void AddAdults(VectorCohort *adults) { AdultQueues.push_back(adults); }
        virtual void AddVectors( const VectorMatingStructure& _vector_genetics, uint64_t releasedNumber );

        // IInfectable
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route ) override;

        // IVectorPopulationReporting
        virtual float  GetEIRByPool(VectorPoolIdEnum::Enum pool_id) const override;
        virtual float  GetHBRByPool(VectorPoolIdEnum::Enum pool_id) const override;
        virtual int32_t getAdultCount()                             const override;
        virtual int32_t getInfectedCount()                          const override;
        virtual int32_t getInfectiousCount()                        const override;
        virtual int32_t getMaleCount()                              const override;
        virtual int32_t getNewEggsCount()                           const override;
        virtual double  getInfectivity()                            const override;
        virtual const std::string& get_SpeciesID()                  const override;
        virtual const VectorHabitatList_t& GetHabitats()            const override;
        virtual std::vector<int> GetNewlyInfectedSuids()            const override;
        virtual std::vector<int> GetInfectiousSuids()               const override;

        virtual const infection_list_t& GetInfections()             const override;
        virtual float GetInterventionReducedAcquire()               const override;

        void SetVectorMortality( bool mortality ) { m_VectorMortality = mortality; }
    protected:
        VectorPopulation();
        void Initialize(INodeContext *context, std::string species_name, unsigned int adults, unsigned int infectious);
        virtual void InitializeVectorQueues(unsigned int adults, unsigned int _infectious);

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

        // Factorize the feeding cycle, which is common to all adult vectors
        virtual uint32_t ProcessFeedingCycle( float dt, IVectorCohort* cohort, VectorStateEnum::Enum state );
        float GetFeedingCycleDurationByTemperature() const;

        // Calculation of mated females based on gender_mating characteristics of male/female populations
        virtual void ApplyMatingGenetics( IVectorCohort* cohort, const VectorMatingStructure& male_vector_genetics );

        // Helper functions for egg calculations
        void CreateEggCohortOfType(        IVectorHabitat*, uint32_t, const VectorMatingStructure& ); // don't need to copy
        void CreateEggCohortAlleleSorting( IVectorHabitat*, uint32_t, VectorMatingStructure ); //need to copy VectorMatingStructure
        void CreateEggCohortHEGSorting(    IVectorHabitat*, uint32_t, VectorMatingStructure ); //need to copy VectorMatingStructure

        // Seek a compatible (same gender mating type) queue in specified list (e.g. AdultQueues, InfectiousQueues) and increase its population.
        virtual void MergeProgressedCohortIntoCompatibleQueue( VectorCohortList_t   &queues, int32_t population, const VectorMatingStructure& _vector_genetics);
        virtual void MergeProgressedCohortIntoCompatibleQueue( VectorCohortVector_t &queues, int32_t population, const VectorMatingStructure& _vector_genetics );

        // Helpers to access information from VectorHabitat to return information about larva
        float GetLarvalDevelopmentProgress (float dt, IVectorCohortWithHabitat* larva) const;
        float GetLarvalMortalityProbability(float dt, IVectorCohortWithHabitat* larva) const;
        float GetRelativeSurvivalWeight(VectorHabitat* habitat) const;

        // VectorPopulation accounting helper function
        virtual void queueIncrementTotalPopulation( IVectorCohort* cohort, VectorStateEnum::Enum state = VectorStateEnum::STATE_ADULT );

        // TODO: Hook these up to configurable parameters?
        //       Now, they are only set to 1.0f in constructor.
        float animalfeed_eggbatchmod;
        float ADfeed_eggbatchmod;

        // List of habitats for this species
        VectorHabitatList_t* m_larval_habitats; // "shared" pointer, NodeVector owns this memory
        std::map<VectorHabitatType::Enum, float> m_larval_capacities;

        int32_t neweggs; 
        int32_t adult;      // female population
        int32_t infected;
        int32_t infectious;
        int32_t males;

        // local variations on base rates
        float dryheatmortality;
        float localadultmortality;
        float infectiouscorrection;

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

        VectorCohortList_t EggQueues;
        VectorCohortList_t LarvaQueues;
        VectorCohortList_t ImmatureQueues;
        VectorCohortVector_t AdultQueues;
        VectorCohortList_t InfectedQueues;
        VectorCohortList_t InfectiousQueues;
        VectorCohortList_t MaleQueues;

        INodeContext                  *m_context;
        const VectorSpeciesParameters *m_species_params;
        VectorProbabilities           *m_probabilities;
        ITransmissionGroups           *m_transmissionGroups;

        bool m_VectorMortality;

        DECLARE_SERIALIZABLE(VectorPopulation);
    };

    // clorton TODO - move these into IArchive
    void serialize(IArchive&, std::pair<float, float>&);
    void serialize(IArchive&, std::map<uint32_t, int>&);
}
