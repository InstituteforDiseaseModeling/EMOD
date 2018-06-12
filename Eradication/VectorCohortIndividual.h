/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorCohort.h"
#include "StrainIdentity.h"

namespace Kernel
{
    struct IVectorCohortIndividual : ISupports
    {
        virtual uint64_t GetID() const = 0 ;
        virtual float GetOvipositionTimer() = 0;
        virtual int GetParity() = 0;
        virtual const IStrainIdentity& GetStrainIdentity() const = 0;
 
        virtual void IncrementParity() = 0 ;
        virtual void ReduceOvipositionTimer( float delta ) = 0;
        virtual void SetOvipositionTimer( float new_opt ) = 0;
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain = NULL, int incubation_period_override = -1) = 0;
        virtual bool IsProgressedOrEmpty() const = 0;
    };

    class VectorCohortIndividual : public VectorCohortAbstract, public IVectorCohortIndividual
    {
    public:
        DECLARE_QUERY_INTERFACE()
        virtual int32_t AddRef() override { return 1; }
        virtual int32_t Release() override { return 1; }

    public:
        static VectorCohortIndividual *CreateCohort( VectorStateEnum::Enum state, 
                                                     float age, 
                                                     float progress, 
                                                     uint32_t initial_population,
                                                     const VectorMatingStructure& vms,
                                                     const std::string* vector_species_name );
        virtual ~VectorCohortIndividual();

        virtual void Merge( IVectorCohort* pCohortToAdd ) override;
        virtual IVectorCohort* Split( uint32_t numLeaving ) override;
        virtual void AddNewEggs( uint32_t daysToGestate, uint32_t new_eggs ) override;
        virtual uint32_t GetGestatedEggs() override;
        virtual void AdjustEggsForDeath( uint32_t numDied ) override;
        virtual const std::vector<uint32_t>& GetNewEggs() const override;

        static void reclaim(IVectorCohortIndividual* ivci);
        static std::vector<VectorCohortIndividual*>* _supply;

        // should we bother with an interface for this common function to individual human? IInfectable?
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain = NULL, int incubation_period_override = -1);
        virtual const IStrainIdentity& GetStrainIdentity() const override;

        virtual uint64_t GetID() const override { return m_ID; }
        virtual float GetOvipositionTimer() override;
        virtual int GetParity() override;
        virtual void IncrementParity() override;
        virtual void ReduceOvipositionTimer( float delta ) override;
        virtual void SetOvipositionTimer( float new_opt ) override;
        virtual bool IsProgressedOrEmpty() const override;

    protected:
        VectorCohortIndividual();
        VectorCohortIndividual( VectorStateEnum::Enum state,
                                float age,
                                float progress,
                                uint32_t initial_population,
                                const VectorMatingStructure& _vector_genetics,
                                const std::string* vector_species_name );

        virtual void Initialize() override;

        uint64_t m_ID ;
        float additional_mortality;
        float oviposition_timer;
        int parity;
        uint32_t neweggs;
        StrainIdentity m_strain;

        DECLARE_SERIALIZABLE(VectorCohortIndividual);
    };
}
