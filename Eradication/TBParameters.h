/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include <map>

namespace Kernel
{
    class TBDrugTypeParameters;

    struct TBParameters
    {
    public:
        std::map< std::string, TBDrugTypeParameters * > TBDrugMap;

        TBParameters()
        : TBDrugMap()
        {
        }
    };
}
