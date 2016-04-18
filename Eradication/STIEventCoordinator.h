/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
    ENUM_DEFINE(BssTargetingType,
        ENUM_VALUE_SPEC(TargetBss               , 0)
        ENUM_VALUE_SPEC(IgnoreBss               , 1)
        ENUM_VALUE_SPEC(NeutralBss              , 2))

    // STI distribution ec that just gives out the intervention once to the fraction of people specified by the coverage parameter
    class STIInterventionDistributionEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, STIInterventionDistributionEventCoordinator, IEventCoordinator)    

    public:
        DECLARE_CONFIGURED(STIInterventionDistributionEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        STIInterventionDistributionEventCoordinator();

        // IEventCoordinator
        //virtual void SetContextTo(ISimulationEventContext *isec);
        
    protected:
        // helpers
        virtual float getDemographicCoverageForIndividual( const IIndividualHumanEventContext *pInd ) const;
        BssTargetingType::Enum bss_targeting;
    };
}
