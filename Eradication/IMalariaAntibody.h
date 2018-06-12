/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <stdint.h>       // for int64_t
#include "MalariaEnums.h" // for MalariaAntibodyType enum
#include "Configuration.h"
#include "ISerializable.h"

namespace Kernel
{
    class SusceptibilityMalariaConfig;

    struct IMalariaAntibody : ISerializable
    {
        // EAW: inv_uL_blood arguments can be removed when we switch to m_antigen_concentration from m_antigen_count
        virtual void  Decay( float dt ) = 0;
        virtual float StimulateCytokines( float dt, float inv_uL_blood ) = 0;
        virtual void  UpdateAntibodyCapacity( float dt, float inv_uL_blood ) = 0;
        virtual void  UpdateAntibodyCapacityByRate( float dt, float growth_rate ) = 0;
        virtual void  UpdateAntibodyConcentration( float dt ) = 0;
        virtual void  ResetCounters() = 0;

        virtual void  IncreaseAntigenCount( int64_t antigenCount ) = 0;
        virtual void  SetAntigenicPresence( bool antigenPresent )  = 0;

        virtual int64_t GetAntigenCount()          const = 0;
        virtual bool    GetAntigenicPresence()     const = 0;
        virtual float   GetAntibodyCapacity()      const = 0;
        virtual float   GetAntibodyConcentration() const = 0;

        virtual void    SetAntibodyCapacity( float antibody_capacity ) = 0;
        virtual void    SetAntibodyConcentration( float antibody_concentration ) = 0;

        virtual MalariaAntibodyType::Enum GetAntibodyType() const = 0;
        virtual int GetAntibodyVariant() const = 0;
    };

    typedef struct
    {
        IMalariaAntibody* minor;
        IMalariaAntibody* major;
    } pfemp1_antibody_t;

    void serialize(IArchive&, pfemp1_antibody_t&);
}
