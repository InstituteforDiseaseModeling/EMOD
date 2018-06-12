/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/
#pragma once
#include <map>

namespace Kernel
{
    class TBHIVDrugTypeParameters;
    struct TBHIVParameters
    {
        //float cd4_count_at_beginning_of_hiv_infection;
        //float cd4_count_at_end_of_hiv_infection;
        std::map< std::string, TBHIVDrugTypeParameters * > TBHIVDrugMap;

        TBHIVParameters()
        : TBHIVDrugMap()
        {
        }
    };
}
