
#include "stdafx.h"
#include "SocietyFactory.h"
#include "SocietyImpl.h"

namespace Kernel {

    ISociety* SocietyFactory::CreateSociety( IRelationshipManager* manager )
    {
        ISociety* pSociety = SocietyImpl::Create( manager );
        return pSociety;
    }
}