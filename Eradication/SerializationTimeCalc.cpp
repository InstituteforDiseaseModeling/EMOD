/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "SerializationTimeCalc.h"
#include "Log.h"
#include "Exceptions.h"
#include "SimulationConfig.h"

using namespace Kernel;

SETUP_LOGGING( "SerializationTimeCalc" )

GET_SCHEMA_STATIC_WRAPPER_IMPL(SerializationTimeCalc, SerializationTimeCalc)
BEGIN_QUERY_INTERFACE_BODY(SerializationTimeCalc)
END_QUERY_INTERFACE_BODY(SerializationTimeCalc)

std::deque<int32_t> SerializationTimeCalc::GetSerializedTimeSteps(int32_t steps)
{
    float start_time = GET_CONFIGURABLE(SimulationConfig)->starttime;
    float step_size = GET_CONFIGURABLE(SimulationConfig)->Sim_Tstep;

    std::vector<int32_t> timesteps;
    switch (m_serialization_type)
    {
    case SerializationType::NONE:
        // Bail early if there's no serialization specified
        return std::deque<int32_t>();

    case SerializationType::TIME:
        // If using times, transform times into timesteps
        for (auto time : m_serialization_times)
        {
            // Special case: don't convert -1: it will become the last timestep
            if (time == -1.0f)
            {
                timesteps.push_back(-1);
            }
            // Special case: don't convert 0: it will become the first timestep
            else if (time == 0.0f)
            {
                timesteps.push_back(0);
            }
            else
            {
                int32_t transformed_time = (int32_t)std::ceil((time - start_time) / step_size);
                timesteps.push_back(transformed_time);
            }
        }
        break;

    case SerializationType::TIMESTEP:
        // If using timesteps, use them directly
        timesteps = m_serialization_time_steps;
        break;

    default:
        // If other enum value specified, throw exception
        throw BadEnumInSwitchStatementException(__FILE__, __LINE__, __FUNCTION__, "m_serialization_type", m_serialization_type, "SerializationType");
    }

    return _ProcessConfig(timesteps, start_time, step_size, steps);
}

std::deque<int32_t> SerializationTimeCalc::_ProcessConfig(std::vector<int32_t> &specified_timesteps, float start_time, float step_size, int32_t steps)
{
    // Replace special values in-place: -1 for last step
    std::replace(specified_timesteps.begin(), specified_timesteps.end(), -1, steps);

    // Sort
    std::sort(specified_timesteps.begin(), specified_timesteps.end());

    // Warn and eliminate duplicates
    std::set<int32_t> times_set(specified_timesteps.begin(), specified_timesteps.end());
    std::list<int32_t> duplicate_times;
    std::set_difference(specified_timesteps.begin(), specified_timesteps.end(),
        times_set.begin(), times_set.end(),
        back_inserter(duplicate_times));
    for (int32_t dupe : duplicate_times)
    {
        LOG_WARN_F("Duplicate serialization step specified (step=%d, time=%f), removed from list\n", dupe, dupe*step_size+start_time);
    }
    specified_timesteps = std::vector<int32_t>(times_set.begin(), times_set.end());

    // Warn and eliminate any too early or too late times
    std::vector<int32_t> bad_times;
    std::copy_if(specified_timesteps.begin(), specified_timesteps.end(), back_inserter(bad_times),
        [&steps](int32_t i) {return i < 0 || i > steps; });
    for (int32_t bad : bad_times)
    {
        LOG_WARN_F("Out of range serialization step specified (step=%d, time=%f), removed from list\n", bad, bad*step_size+start_time);
        specified_timesteps.erase(std::remove(specified_timesteps.begin(), specified_timesteps.end(), bad), specified_timesteps.end());
    }
        
    // Return as a deque
    return std::deque<int32_t>(specified_timesteps.begin(), specified_timesteps.end());
}

bool SerializationTimeCalc::Configure(const Configuration * inputJson)
{
    initConfig("Serialization_Type", m_serialization_type, inputJson, MetadataDescriptor::Enum("Serialization_Type", Serialization_Type_DESC_TEXT, MDD_ENUM_ARGS(SerializationType)));
    initConfigTypeMap("Serialization_Time_Steps", &m_serialization_time_steps, Serialization_Time_Steps_DESC_TEXT, -1, INT_MAX, 0, "Serialization_Type", "TIMESTEP");
    initConfigTypeMap("Serialization_Times", &m_serialization_times, Serialization_Times_DESC_TEXT, -1, FLT_MAX, 0, false, "Serialization_Type", "TIME");

    return JsonConfigurable::Configure(inputJson);
}