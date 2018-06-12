/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ISupports.h"

namespace Kernel
{
    boost::uuids::string_generator string_gen;

    boost::uuids::name_generator TypeInfoHelper::dtk_name_gen(string_gen("{6ba7b810-9dad-11d1-80b4-00c04fd430c8}")); // generate dns-based guids, this guid is the dns namespace guid; see boost docs for example this was derived from
}