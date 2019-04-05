/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorEnums.h"
#include "IVectorHabitat.h"

namespace Kernel
{
    struct IStrainIdentity;
    struct IVectorCohort;
    class  VectorHabitat;
    class  VectorMatingStructure;
    struct IVectorPopulationReporting;
    class  VectorProbabilities;

    typedef std::list<IVectorPopulationReporting *> VectorPopulationReportingList_t;

    struct IVectorSimulationContext : public ISupports
    {
        virtual void  PostMigratingVector( const suids::suid& nodeSuid, IVectorCohort* ind ) = 0;
        virtual float GetNodePopulation( const suids::suid& nodeSuid ) = 0;
        virtual float GetAvailableLarvalHabitat( const suids::suid& nodeSuid, const std::string& rSpeciesID ) = 0 ;
    };

    struct IVectorNodeContext : public ISupports
    {
        virtual VectorProbabilities* GetVectorLifecycleProbabilities() = 0;
        virtual IVectorHabitat*      GetVectorHabitatBySpeciesAndType( std::string& species, VectorHabitatType::Enum type, const Configuration* inputJson ) = 0;
        virtual VectorHabitatList_t* GetVectorHabitatsBySpecies( std::string& species ) = 0;
        virtual float                GetLarvalHabitatMultiplier( VectorHabitatType::Enum type, const std::string& species ) const = 0;
    };

    // TODO: merge the two NodeVector interfaces?  or split functionally?
    class INodeVector : public ISupports
    {
    public:
        virtual const VectorPopulationReportingList_t& GetVectorPopulationReporting() const = 0;
        virtual void AddVectors( const std::string& releasedSpecies, const VectorMatingStructure& _vector_genetics, uint32_t releasedNumber ) = 0;
        virtual void processImmigratingVector( IVectorCohort* immigrant ) = 0;
    };

    struct IIndividualHumanVectorContext : public ISupports
    {
        virtual float GetRelativeBitingRate(void) const = 0;
    };

    struct IVectorSusceptibilityContext : public ISupports
    {
        virtual void  SetRelativeBitingRate( float rate ) = 0;
        virtual float GetRelativeBitingRate(void) const = 0;
    };

    struct IVectorInterventionsEffects : ISupports
    {
        virtual float GetDieBeforeFeeding() = 0;
        virtual float GetHostNotAvailable() = 0;
        virtual float GetDieDuringFeeding() = 0;
        virtual float GetDiePostFeeding() = 0;
        virtual float GetSuccessfulFeedHuman() = 0;
        virtual float GetSuccessfulFeedAD() = 0;
        virtual float GetOutdoorDieBeforeFeeding() = 0;
        virtual float GetOutdoorHostNotAvailable() = 0;
        virtual float GetOutdoorDieDuringFeeding() = 0;
        virtual float GetOutdoorDiePostFeeding() = 0;
        virtual float GetOutdoorSuccessfulFeedHuman() = 0;
        virtual float GetblockIndoorVectorAcquire() = 0;
        virtual float GetblockIndoorVectorTransmit() = 0;
        virtual float GetblockOutdoorVectorAcquire() = 0;
        virtual float GetblockOutdoorVectorTransmit() = 0;
        virtual ~IVectorInterventionsEffects() { }
    };

    struct INodeVectorInterventionEffects : ISupports
    {
        virtual float GetLarvalKilling(VectorHabitatType::Enum) = 0;
        virtual float GetLarvalHabitatReduction(VectorHabitatType::Enum, const std::string& species) = 0;
        virtual float GetVillageSpatialRepellent() = 0;
        virtual float GetADIVAttraction() = 0;
        virtual float GetADOVAttraction() = 0;
        virtual float GetPFVKill() = 0;
        virtual float GetOutdoorKilling() = 0;
        virtual float GetOutdoorKillingMale() = 0;
        virtual float GetSugarFeedKilling() = 0;
        virtual float GetOviTrapKilling(VectorHabitatType::Enum) = 0;
        virtual float GetAnimalFeedKilling() = 0;
        virtual float GetOutdoorRestKilling() = 0;
        virtual float GetIndoorKilling() = 0;
    };

    struct IVectorPopulationReporting : ISupports
    {
        virtual float GetEIRByPool(VectorPoolIdEnum::Enum pool_id)      const = 0;
        virtual float GetHBRByPool(VectorPoolIdEnum::Enum pool_id)      const = 0;
        virtual uint32_t getAdultCount()                                const = 0;
        virtual uint32_t getInfectedCount(   IStrainIdentity* pStrain ) const = 0;
        virtual uint32_t getInfectiousCount( IStrainIdentity* pStrain ) const = 0;
        virtual uint32_t getMaleCount()                                 const = 0;
        virtual uint32_t getNewEggsCount()                              const = 0;
        virtual uint32_t getNewAdults()                                 const = 0;
        virtual uint32_t getNumDiedBeforeFeeding()                      const = 0;
        virtual uint32_t getNumDiedDuringFeedingIndoor()                const = 0;
        virtual uint32_t getNumDiedDuringFeedingOutdoor()               const = 0;
        virtual double  getInfectivity()                                const = 0;
        virtual const std::string& get_SpeciesID()                      const = 0;
        virtual const VectorHabitatList_t& GetHabitats()                const = 0;
        virtual std::vector<uint64_t> GetNewlyInfectedVectorIds()       const = 0;
        virtual std::vector<uint64_t> GetInfectiousVectorIds()          const = 0;
    };
}
