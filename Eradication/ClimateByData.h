/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>

#include "BoostLibWrapper.h"

#include "RANDOM.h"
#include "Contexts.h"
#include "Climate.h"

namespace Kernel
{
    class ClimateByData : public Climate
    {
        GET_SCHEMA_STATIC_WRAPPER(ClimateByData)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        bool Configure( const Configuration* config );

    protected:
        int num_datapoints;
        int num_years;

        // vectors containing the entire timeline of data
        std::vector<float> airtemperature_data;
        std::vector<float> landtemperature_data;
        std::vector<float> rainfall_data;
        std::vector<float> humidity_data;

    public:
        static ClimateByData * CreateClimate(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent, int datapoints, std::ifstream& airtemperature_file, std::ifstream& landtemperature_file, std::ifstream& rainfall_file, std::ifstream& humidity_file, float start_time);

        virtual void UpdateWeather(float, float);
        virtual ~ClimateByData();

    private:
        ClimateByData();
        ClimateByData(ClimateUpdateResolution::Enum update_resolution, INodeContext * _parent);

        virtual bool IsPlausible();

        void ReadDataFromFiles(int datapoints, std::ifstream& airtemperature_file, std::ifstream& landtemperature_file, std::ifstream& rainfall_file, std::ifstream& humidity_file);
    };
}
