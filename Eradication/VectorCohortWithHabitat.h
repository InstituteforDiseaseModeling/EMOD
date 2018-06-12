/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorCohort.h"

namespace Kernel
{
    struct IVectorHabitat;

    class IVectorCohortWithHabitat : public ISupports
    {
    public:
        virtual VectorHabitatType::Enum GetHabitatType() = 0;
        virtual IVectorHabitat* GetHabitat() = 0;
        virtual void SetHabitat( IVectorHabitat* ) = 0;
    };

    class VectorCohortWithHabitat : public VectorCohort, public IVectorCohortWithHabitat
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorCohortWithHabitat *CreateCohort( IVectorHabitat* _habitat,
                                                      VectorStateEnum::Enum state,
                                                      float progress,
                                                      int32_t initial_population,
                                                      const VectorMatingStructure& _vector_genetics,
                                                      const std::string* vector_species_name );
        virtual ~VectorCohortWithHabitat();

        virtual VectorHabitatType::Enum GetHabitatType();
        virtual IVectorHabitat* GetHabitat();
        virtual void SetHabitat( IVectorHabitat* );

    protected:
        IVectorHabitat* habitat;
        VectorHabitatType::Enum habitat_type;
        VectorCohortWithHabitat();
        VectorCohortWithHabitat( IVectorHabitat* _habitat,
                                 VectorStateEnum::Enum state,
                                 float progress,
                                 int32_t initial_population,
                                 const VectorMatingStructure& _vector_genetics,
                                 const std::string* vector_species_name );
        virtual void Initialize() override;

        DECLARE_SERIALIZABLE(VectorCohortWithHabitat);
    };
}
