/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <list>
#include "suids.hpp"
#include "IMigrate.h"
#include "Vector.h"
#include "VectorMatingStructure.h"
#include "ISerializable.h"
#include "IVectorCohort.h"

namespace Kernel
{
    struct IStrainIdentity;    
    struct INodeContext;

    class VectorCohortAbstract : public IVectorCohort, public IMigrate
    {
    public:
        virtual int32_t AddRef() override { return 1; }
        virtual int32_t Release() override { return 1; }
        DECLARE_QUERY_INTERFACE()

    public:
        static std::string _gambiae;

        virtual ~VectorCohortAbstract();

        virtual const IStrainIdentity& GetStrainIdentity() const override;

        // IMigrate interfaces
        virtual void ImmigrateTo(INodeContext* destination_node) override;
        virtual void SetMigrating( suids::suid destination, 
                                   MigrationType::Enum type, 
                                   float timeUntilTrip, 
                                   float timeAtDestination,
                                   bool isDestinationNewHome ) override;
        virtual const suids::suid & GetMigrationDestination() override;
        virtual MigrationType::Enum GetMigrationType() const  override;

        virtual VectorStateEnum::Enum GetState() const override;
        virtual void SetState( VectorStateEnum::Enum _state ) override;
        virtual const std::string &GetSpecies() override;
        virtual uint32_t GetPopulation() const override; // used by VPI
        virtual void SetPopulation( uint32_t new_pop ) override; // used by VPI (1x besides ClearPop)
        virtual float GetProgress() const override; // NOT used by VPI
        virtual void ClearProgress() override; // NOT used by VPI, implicit
        virtual void IncreaseProgress( float delta ) override; // used by VPI (2x)
        virtual VectorMatingStructure& GetVectorGenetics() override; // used by VPI
        virtual void SetVectorGenetics( const VectorMatingStructure& new_value ) override;
        virtual IMigrate* GetIMigrate() override;
        virtual float GetAge() const override;
        virtual void SetAge( float ageDays ) override;
        virtual void IncreaseAge( float dt ) override;

    protected:
        VectorCohortAbstract();
        VectorCohortAbstract( const VectorCohortAbstract& rThat );
        VectorCohortAbstract( VectorStateEnum::Enum state,
                      float age,
                      float progress,
                      uint32_t population, 
                      const VectorMatingStructure& _vector_genetics,
                      const std::string* vector_species_name );
        virtual void Initialize();

        VectorMatingStructure vector_genetics;
        VectorStateEnum::Enum state;
        float progress;
        uint32_t population;
        float age;
        MigrationType::Enum migration_type;
        suids::suid migration_destination;
        const std::string* pSpecies;

        static void serialize( IArchive& ar, VectorCohortAbstract* obj );
    };

    class VectorCohort : public VectorCohortAbstract
    {
    public:
        DECLARE_QUERY_INTERFACE()

        static VectorCohort *CreateCohort( VectorStateEnum::Enum state,
                                           float age,
                                           float progress,
                                           uint32_t population,
                                           const VectorMatingStructure& vms,
                                           const std::string* vector_species_name );
        virtual ~VectorCohort();

        virtual void Merge( IVectorCohort* pCohortToAdd ) override;
        virtual IVectorCohort* Split( uint32_t numLeaving ) override;
        virtual void AddNewEggs( uint32_t daysToGestate, uint32_t new_eggs ) override;
        virtual uint32_t GetGestatedEggs() override;
        virtual void AdjustEggsForDeath( uint32_t numDied ) override;
        virtual const std::vector<uint32_t>& GetNewEggs() const override;

    protected:
        VectorCohort();
        VectorCohort( const VectorCohort& rThat );
        VectorCohort( VectorStateEnum::Enum state,
                      float age,
                      float progress,
                      uint32_t population,
                      const VectorMatingStructure& _vector_genetics,
                      const std::string* vector_species_name );

        std::vector<uint32_t> neweggs;

        DECLARE_SERIALIZABLE(VectorCohort);
    };
}
