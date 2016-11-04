/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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

namespace Kernel
{
    class StrainIdentity;
    
    struct IVectorCohort : ISerializable
    {
        /* Only for type-ID right now */
        virtual int32_t GetPopulation() const = 0;
        virtual void SetPopulation( int32_t new_pop ) = 0;
        virtual double GetProgress() const = 0;
        virtual void ClearProgress() = 0;
        virtual void IncreaseProgress( double delta ) = 0;
        virtual VectorMatingStructure& GetVectorGenetics() = 0;
        virtual void SetVectorGenetics( const VectorMatingStructure& new_value ) = 0;
        virtual float GetMortality( uint32_t addition ) const = 0;
        virtual const StrainIdentity* GetStrainIdentity() const = 0;
        virtual IMigrate* GetIMigrate() = 0;
    };

    struct INodeContext;

    struct VectorCohort;
    typedef std::list<IVectorCohort *> VectorCohortList_t;

    struct VectorCohort : IVectorCohort, IMigrate
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorCohort *CreateCohort( float progress = 0.0, uint32_t population = DEFAULT_VECTOR_COHORT_SIZE, VectorMatingStructure = VectorMatingStructure() );
        virtual ~VectorCohort();

        virtual const StrainIdentity* GetStrainIdentity() const override;

        // IMigrate interfaces
        virtual void ImmigrateTo(INodeContext* destination_node) override;
        virtual void SetMigrating( suids::suid destination, 
                                   MigrationType::Enum type, 
                                   float timeUntilTrip, 
                                   float timeAtDestination,
                                   bool isDestinationNewHome ) override;
        virtual const suids::suid & GetMigrationDestination() override;
        virtual MigrationType::Enum GetMigrationType() const  override;

        virtual int32_t GetPopulation() const override; // used by VPI
        virtual void SetPopulation( int32_t new_pop ) override; // used by VPI (1x besides ClearPop)
        virtual double GetProgress() const override; // NOT used by VPI
        virtual void ClearProgress() override; // NOT used by VPI, implicit
        virtual void IncreaseProgress( double delta ) override; // used by VPI (2x)
        virtual VectorMatingStructure& GetVectorGenetics() override; // used by VPI
        virtual void SetVectorGenetics( const VectorMatingStructure& new_value ) override;
        virtual float GetMortality( uint32_t addition ) const override;
        virtual IMigrate* GetIMigrate() override;

    protected:
        VectorCohort();
        VectorCohort(float progress, uint32_t population, VectorMatingStructure _vector_genetics);
        virtual void Initialize();

        VectorMatingStructure vector_genetics;
        double progress;
        int32_t population;

        DECLARE_SERIALIZABLE(VectorCohort);
    };
}
