/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include <stdafx.h>
#include "IdmDateTime.h"
#include "Log.h"

namespace Kernel {
    static const char * _module = "IdmDateTime";
    NonNegativeFloat IdmDateTime::_base_year = 0;
}
