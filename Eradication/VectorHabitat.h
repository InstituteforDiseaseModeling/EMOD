/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <stdint.h>
#include <list>
#include <vector>

#include "VectorEnums.h"

namespace Kernel
{
    // Current timestep and immediately previous for total larva counts
    enum TimeStepIndex
    {
        CURRENT_TIME_STEP           = 0,
        PREVIOUS_TIME_STEP          = 1,
    };

    struct INodeContext;
    class  SimulationConfig;

    struct IVectorHabitat
    {
        virtual VectorHabitatType::Enum  GetVectorHabitatType()                   const = 0;
        virtual float                    GetMaximumLarvalCapacity()               const = 0;
        virtual float                    GetCurrentLarvalCapacity()               const = 0;
        virtual int32_t                  GetTotalLarvaCount(TimeStepIndex index)  const = 0;

        virtual float                    GetOvipositionTrapKilling()     const = 0;
        virtual float                    GetArtificialLarvalMortality()  const = 0;
        virtual float                    GetLarvicideHabitatScaling()    const = 0;
        virtual float                    GetRainfallMortality()          const = 0;
        virtual float                    GetEggCrowdingCorrection()      const = 0;

        virtual float GetLocalLarvalGrowthModifier() const = 0;
        virtual float GetLocalLarvalMortality(float species_aquatic_mortality, float progress) const = 0;
    };

    class VectorHabitat : public IVectorHabitat
    {
    public:
        static VectorHabitat* CreateHabitat( VectorHabitatType::Enum type, float max_capacity );
        virtual ~VectorHabitat();
        void Update(float dt, INodeContext* node);

        virtual VectorHabitatType::Enum  GetVectorHabitatType()                   const;
        virtual float                    GetMaximumLarvalCapacity()               const;
        virtual float                    GetCurrentLarvalCapacity()               const;
        virtual int32_t                  GetTotalLarvaCount(TimeStepIndex index)  const;

        void                             AddLarva(int32_t larva, float progress);
        void                             AddEggs(int32_t eggs);
        void                             IncrementMaxLarvalCapacity(float);

        virtual float                    GetOvipositionTrapKilling()     const;
        virtual float                    GetArtificialLarvalMortality()  const;
        virtual float                    GetLarvicideHabitatScaling()    const;
        virtual float                    GetRainfallMortality()          const;
        virtual float                    GetEggCrowdingCorrection()      const;

        virtual float GetLocalLarvalGrowthModifier() const;
        virtual float GetLocalLarvalMortality(float species_aquatic_mortality, float progress) const;

    protected:
        VectorHabitat( VectorHabitatType::Enum type, float max_capacity );

        void CalculateEggCrowdingCorrection();
        void UpdateCurrentLarvalCapacity(float dt, INodeContext* node);
        void UpdateLarvalProbabilities(float dt, INodeContext* node);
        void UpdateRainfallMortality(float dt, float rainfall);
        void AdvanceTotalLarvaCounts();

        const SimulationConfig* params() const;

        VectorHabitatType::Enum  m_habitat_type;
        float                    m_max_larval_capacity;
        float                    m_current_larval_capacity;
        std::vector<int32_t>     m_total_larva_count;
        int32_t                  m_new_egg_count;

        float                    m_oviposition_trap_killing;
        float                    m_artificial_larval_mortality;
        float                    m_larvicide_habitat_scaling;
        float                    m_rainfall_mortality;
        float                    m_egg_crowding_correction;
    };

    typedef std::list<VectorHabitat *> VectorHabitatList_t;
}
