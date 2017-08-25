/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
namespace Kernel {

    ENUM_DEFINE( TyphoidVaccineMode,
        ENUM_VALUE_SPEC( Shedding,  1 )
        ENUM_VALUE_SPEC( Dose,      2 )
        ENUM_VALUE_SPEC( Exposures, 3 )
    )
}
