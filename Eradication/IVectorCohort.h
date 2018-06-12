/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ISerializable.h"
#include "VectorEnums.h"

namespace Kernel
{
    class VectorMatingStructure;
    struct IMigrate;
    struct IStrainIdentity;

    struct IVectorCohort : ISerializable
    {
        virtual VectorStateEnum::Enum GetState() const = 0;
        virtual void SetState( VectorStateEnum::Enum _state ) = 0;
        virtual const std::string &GetSpecies() = 0;
        virtual uint32_t GetPopulation() const = 0;
        virtual void SetPopulation( uint32_t new_pop ) = 0;
        virtual float GetProgress() const = 0;
        virtual void ClearProgress() = 0;
        virtual void IncreaseProgress( float delta ) = 0;
        virtual VectorMatingStructure& GetVectorGenetics() = 0;
        virtual void SetVectorGenetics( const VectorMatingStructure& new_value ) = 0;
        virtual const IStrainIdentity& GetStrainIdentity() const = 0;
        virtual IMigrate* GetIMigrate() = 0;
        virtual float GetAge() const = 0;
        virtual void SetAge( float ageDays ) = 0;
        virtual void IncreaseAge( float dt ) = 0;
        virtual void AddNewEggs( uint32_t daysToGestate, uint32_t new_eggs ) = 0;
        virtual uint32_t GetGestatedEggs() = 0;
        virtual void AdjustEggsForDeath( uint32_t numDied ) = 0;
        virtual void Merge( IVectorCohort* pCohortToAdd ) = 0;
        virtual IVectorCohort* Split( uint32_t numLeaving ) = 0;
        virtual const std::vector<uint32_t>& GetNewEggs() const = 0;
    };

    typedef std::vector<IVectorCohort*> VectorCohortVector_t;

}
