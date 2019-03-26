/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "Configure.h"
#include "EnumSupport.h"
#include <deque>
#include <vector>

using namespace Kernel;

ENUM_DEFINE(SerializationType,
    ENUM_VALUE_SPEC(NONE, 0)
    ENUM_VALUE_SPEC(TIME, 1)
    ENUM_VALUE_SPEC(TIMESTEP, 2))

class SerializationTimeCalc : public JsonConfigurable
{
IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
DECLARE_QUERY_INTERFACE()
GET_SCHEMA_STATIC_WRAPPER(SerializationTimeCalc)
public:
    std::deque<int32_t> GetSerializedTimeSteps(int32_t steps);
    virtual bool Configure(const Configuration *config);
    
private:
    std::deque<int32_t> _ProcessConfig(std::vector<int32_t> &specified_timesteps, float start_time, float step_size, int32_t steps);

    SerializationType::Enum m_serialization_type;
    std::vector<int32_t> m_serialization_time_steps;
    std::vector<float> m_serialization_times;
};
