
#pragma once
#include "IdmApi.h"
#include "ISociety.h"
#include "IRelationshipManager.h"

namespace Kernel {

    class SocietyFactory {
    public:
        IDMAPI static ISociety* CreateSociety( IRelationshipManager* );
    };
}