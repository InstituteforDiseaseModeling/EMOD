/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "RateTableFactory.h"
#include "RateTableImpl.h"

namespace Kernel  {

    IPairFormationRateTable* Kernel::RateTableFactory::CreateRateTable( const IPairFormationParameters* parameters )
    {
        IPairFormationRateTable* prt = RateTableImpl::CreateRateTable(parameters);
        return prt;
    }
}
