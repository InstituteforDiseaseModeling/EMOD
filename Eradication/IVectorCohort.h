
#pragma once

#include "ISerializable.h"
#include "VectorEnums.h"

namespace Kernel
{
    class RANDOMBASE;
    class VectorGenome;
    class VectorTraitModifiers;
    struct IMigrate;
    struct IStrainIdentity;
    struct IVectorHabitat;

    struct IVectorCohort : ISerializable
    {
        virtual uint32_t GetID() const = 0;
        virtual int GetSpeciesIndex() const = 0;
        virtual const VectorGenome& GetGenome() const = 0;
        virtual void SetMateGenome( const VectorGenome& rGenomeMate ) = 0;
        virtual const VectorGenome& GetMateGenome() const = 0;
        virtual bool HasMated() const = 0;
        virtual VectorStateEnum::Enum GetState() const = 0;
        virtual void SetState( VectorStateEnum::Enum _state ) = 0;
        virtual const std::string &GetSpecies() = 0;
        virtual VectorWolbachia::Enum GetWolbachia() const = 0;
        virtual uint32_t GetPopulation() const = 0;
        virtual void SetPopulation( uint32_t new_pop ) = 0;
        virtual float GetProgress() const = 0;
        virtual void ClearProgress() = 0;
        virtual IMigrate* GetIMigrate() = 0;
        virtual float GetAge() const = 0;
        virtual void SetAge( float ageDays ) = 0;
        virtual void IncreaseAge( float dt ) = 0;
        virtual uint32_t GetNumLookingToFeed() const = 0;
        virtual void AddNewGestating( uint32_t daysToGestate, uint32_t newGestating ) = 0;
        virtual uint32_t GetNumGestating() const = 0;
        virtual uint32_t RemoveNumDoneGestating() = 0;
        virtual uint32_t AdjustGestatingForDeath( RANDOMBASE* pRNG, float percentDied, bool killGestatingOnly ) = 0;
        virtual const std::vector<uint32_t>& GetGestatingQueue() const = 0;
        virtual void ReportOnGestatingQueue( std::vector<uint32_t>& rNumGestatingQueue ) const = 0;
        virtual void Merge( IVectorCohort* pCohortToAdd ) = 0;
        virtual IVectorCohort* SplitPercent( RANDOMBASE* pRNG, uint32_t newVectorID, float percentLeaving ) = 0;
        virtual IVectorCohort* SplitNumber( RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving ) = 0;
        virtual void Update( RANDOMBASE* pRNG,
                             float dt,
                             const VectorTraitModifiers& rTraitModifiers,
                             float progressThisTimestep,
                             bool hadMicrosporidiaPreviously ) = 0;
        virtual VectorHabitatType::Enum GetHabitatType() = 0;
        virtual IVectorHabitat* GetHabitat() = 0;
        virtual void SetHabitat( IVectorHabitat* ) = 0;
        virtual bool HasWolbachia() const = 0;
        virtual bool HasMicrosporidia() const = 0;
        virtual void InfectWithMicrosporidia( int strainIndex ) = 0;
        virtual float GetDurationOfMicrosporidia() const = 0;
    };

    typedef std::vector<IVectorCohort*> VectorCohortVector_t;

}
