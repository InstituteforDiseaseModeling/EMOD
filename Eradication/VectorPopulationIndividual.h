/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

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
        VectorPopulationIndividual() : m_mosquito_weight(0), m_average_oviposition_killing(0) {};
        static VectorPopulationIndividual *CreatePopulation( INodeContext *context, const std::string& species_name, uint32_t _adult, uint32_t _infectious, uint32_t mosquito_weight );
        virtual ~VectorPopulationIndividual() override;

        // IVectorPopulation
        virtual void Vector_Migration( float dt, IMigrationInfo* pMigInfo, VectorCohortVector_t* pMigratingQueue ) override;
        virtual void AddImmigratingVector( IVectorCohort* pvc ) override;

        // IInfectable
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route ) override;

        // IVectorPopulationReporting
        virtual uint32_t getInfectedCount(   IStrainIdentity* pStrain ) const override;
        virtual uint32_t getInfectiousCount( IStrainIdentity* pStrain ) const override;
        virtual std::vector<uint64_t> GetNewlyInfectedVectorIds()   const override;
        virtual std::vector<uint64_t> GetInfectiousVectorIds()      const override;

    protected:
        VectorPopulationIndividual(uint32_t mosquito_weight);
        virtual void InitializeVectorQueues( uint32_t adults, uint32_t _infectious ) override;
        virtual uint32_t ProcessFeedingCycle( float dt, IVectorCohort* cohort ) override;
        void ExposeCohortList( const IContagionPopulation* cp, VectorCohortVector_t& list, float success_prob, float infection_prob );
        uint32_t CalculateOvipositionTime();
        void RandomlySetOvipositionTimer( VectorCohortIndividual* pvci );

        virtual void Update_Infectious_Queue( float dt ) override;
        virtual void Update_Infected_Queue( float dt ) override;
        virtual void Update_Adult_Queue( float dt ) override;

        virtual void AddAdultsFromMating( const VectorGeneticIndex_t& rVgiMale,
                                          const VectorGeneticIndex_t& rVgiFemale,
                                          uint32_t pop ) override;

        // function to support adding in new vectors (Wolbachia types, etc...)
        virtual void AddVectors_Adults( const VectorMatingStructure& _vector_genetics, uint32_t releasedNumber ) override;

        virtual void AdjustForFeedingRate( float dt, float p_local_mortality, FeedingProbabilities& rFeedProbs );
        virtual void AdjustForCumulativeProbability( FeedingProbabilities& rFeedProbs );

        virtual void VectorToHumanDeposit( const IStrainIdentity& strain,
                                           uint32_t attemptFeed,
                                           const TransmissionGroupMembership_t* pTransmissionVectorToHuman );

        bool DidDie(  IVectorCohort* cohort, float probDies, float outcome, uint32_t& rCounter );
        bool DidFeed( IVectorCohort* cohort, float probFeed, float outcome, uint32_t& rCounter );
        bool DiedLayingEggs( IVectorCohort* cohort );

        virtual void GenerateEggs( uint32_t numFeedHuman, uint32_t numFeedAD, uint32_t numFeedAnimal, IVectorCohort* cohort ) override;


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
