/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorCohortAging.h"

namespace Kernel
{
    struct IVectorCohortIndividual : ISupports
    {
        virtual uint64_t GetID() const = 0 ;
        virtual VectorStateEnum::Enum & GetState() = 0;
        virtual void SetState( const VectorStateEnum::Enum & ) = 0;
        virtual float GetAdditionalMortality() const = 0;
        virtual float GetOvipositionTimer() = 0;
        virtual int GetParity() = 0;
        virtual int GetNewEggs() = 0;
        virtual const std::string &GetSpecies() = 0;
        virtual const suids::suid & GetMigrationDestination() = 0;
        virtual const StrainIdentity* GetStrainIdentity() const = 0;
 
        virtual void IncrementParity() = 0 ;
        virtual void ReduceOvipositionTimer( float delta ) = 0;
        virtual void SetAdditionalMortality( float new_mortality ) = 0;
        virtual void SetNewEggs( int new_eggs ) = 0;
        virtual void SetOvipositionTimer( float new_opt ) = 0;
        virtual void AcquireNewInfection( StrainIdentity *infstrain = nullptr, int incubation_period_override = -1 ) = 0;
        virtual bool IsProgressedOrEmpty() const = 0;
        virtual float GetMortality( uint32_t addition ) const = 0;

        // VectorCohortAging base class wrappers
        virtual float GetAge() const = 0;
        virtual void IncreaseAge( float dt ) = 0;
    };

    class VectorCohortIndividual : public VectorCohortAging, public IVectorCohortIndividual
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorCohortIndividual *CreateCohort(VectorStateEnum::Enum state = VectorStateEnum::STATE_ADULT, float age = 0.0f, float progress = 0.0f, int32_t initial_population = 100, VectorMatingStructure = VectorMatingStructure(), std::string vector_species_name = "gambiae");
        virtual ~VectorCohortIndividual();

        // should we bother with an interface for this common function to individual human? IInfectable?
        virtual void AcquireNewInfection( StrainIdentity *infstrain = nullptr, int incubation_period_override = -1 ) override;
        virtual const StrainIdentity* GetStrainIdentity() const override;

        // IMigrate interfaces
        virtual void ImmigrateTo(INodeContext* destination_node) override;
        virtual void SetMigrating( suids::suid destination, 
                                   MigrationType::Enum type, 
                                   float timeUntilTrip, 
                                   float timeAtDestination,
                                   bool isDestinationNewHome ) override;
        virtual const suids::suid& GetMigrationDestination() override;
        virtual MigrationType::Enum GetMigrationType() const override { return migration_type ; }

        virtual uint64_t GetID() const override { return m_ID; }
        virtual VectorStateEnum::Enum & GetState() override;
        virtual void SetState( const VectorStateEnum::Enum & ) override;
        virtual float GetAdditionalMortality() const override;
        virtual float GetOvipositionTimer() override;
        virtual int GetParity() override;
        virtual int GetNewEggs() override;
        virtual const std::string &GetSpecies() override;
        virtual void IncrementParity() override;
        virtual void ReduceOvipositionTimer( float delta ) override;
        virtual void SetAdditionalMortality( float new_mortality ) override;
        virtual void SetNewEggs( int new_eggs ) override;
        virtual void SetOvipositionTimer( float new_opt ) override;
        virtual bool IsProgressedOrEmpty() const override;

        virtual float GetMortality( uint32_t addition ) const override;

        // base class wrappers
        virtual float GetAge() const override;
        virtual void IncreaseAge( float dt ) override;

    protected:
        VectorCohortIndividual();
        VectorCohortIndividual(VectorStateEnum::Enum state, float age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics, std::string vector_species_name);
        /* clorton virtual */ void Initialize() /* clorton override */;

        uint64_t m_ID ;
        VectorStateEnum::Enum state;
        float additional_mortality;
        float oviposition_timer;
        int parity;
        int neweggs;
        suids::suid migration_destination;
        MigrationType::Enum migration_type ;
        std::string species;

        StrainIdentity* m_strain;

        DECLARE_SERIALIZABLE(VectorCohortIndividual);
    };
}
