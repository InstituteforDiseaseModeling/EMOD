/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

namespace Kernel
{
    class IndividualHuman ;
    struct IDrug ;
}


namespace ReportUtilities
{

    std::list<Kernel::IDrug*> GetDrugList( const Kernel::IndividualHuman * individual, const std::string& rDrugClassName );

    int GetAgeBin( float age, std::vector<float>& rAges );
}