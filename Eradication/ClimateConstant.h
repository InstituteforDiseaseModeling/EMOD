/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>

#include "BoostLibWrapper.h"

#include "Climate.h"

namespace Kernel
{
    class RANDOMBASE;

    class ClimateConstant : public Climate
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER(ClimateConstant)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        bool Configure( const Configuration* config );
        static ClimateConstant * CreateClimate( ClimateUpdateResolution::Enum update_resolution,
                                                INodeContext * _parent,
                                                float start_time,
                                                RANDOMBASE* pRNG );

        virtual void UpdateWeather( float, float, RANDOMBASE* pRNG ) override;

    protected:
        ClimateConstant();
        ClimateConstant(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent);

        virtual bool IsPlausible();
    };
}
