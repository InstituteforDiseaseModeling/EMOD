/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Contexts.h"
#include "VectorEnums.h"

namespace Kernel
{
    struct VectorCohort;
    class  VectorHabitat;
    class  VectorMatingStructure;
    class  VectorPopulation;
    class  VectorProbabilities;

    typedef std::list<VectorPopulation *> VectorPopulationList_t;

    struct IVectorSimulationContext : public ISupports
    {
        virtual void  PostMigratingVector(VectorCohort* ind) = 0;
    };

    struct IVectorNodeContext : public ISupports
    {
        virtual VectorProbabilities* GetVectorLifecycleProbabilities() = 0;
        virtual VectorHabitat*       GetVectorHabitatByType(VectorHabitatType::Enum type) = 0;
        virtual void                 AddVectorHabitat(VectorHabitat* habitat) = 0;
        virtual float                GetLarvalHabitatMultiplier(VectorHabitatType::Enum type) const = 0;
    };

    // TODO: merge the two NodeVector interfaces?  or split functionally?
    class INodeVector : public ISupports
    {
    public:
        virtual const VectorPopulationList_t& GetVectorPopulations() = 0;
        virtual void AddVectors(std::string releasedSpecies, VectorMatingStructure _vector_genetics, unsigned long int releasedNumber) = 0;
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
}
