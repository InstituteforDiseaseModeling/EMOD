/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "StandardEventCoordinator.h"

namespace Kernel
{

    class GroupInterventionDistributionEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, GroupInterventionDistributionEventCoordinator, IEventCoordinator)    
    public:
        DECLARE_CONFIGURED(GroupInterventionDistributionEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        GroupInterventionDistributionEventCoordinator();
        // IEventCoordinator

        virtual bool qualifiesDemographically( const IIndividualHumanEventContext * pIndividual ) override;


    protected:

        TargetGroupType::Enum target_disease_state;
    };
}
