
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
