/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Contexts.h"
#include "VectorEnums.h"
#include "IVectorHabitat.h"

namespace Kernel
{
    struct VectorCohort;
    struct IVectorCohort;
    class  VectorHabitat;
    class  VectorMatingStructure;
    class  VectorPopulation;
    class  VectorProbabilities;

    typedef std::list<VectorPopulation *> VectorPopulationList_t;

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
        virtual const VectorPopulationList_t& GetVectorPopulations() = 0;
        virtual void AddVectors(std::string releasedSpecies, VectorMatingStructure _vector_genetics, uint64_t releasedNumber) = 0;
        virtual void processImmigratingVector( VectorCohort* immigrant ) = 0;
    };

    struct IIndividualHumanVectorContext : public ISupports
    {
        virtual float GetRelativeBitingRate(void) const = 0;
    };

    struct IVectorSusceptibilityContext : public ISupports
    {
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
    };
}
