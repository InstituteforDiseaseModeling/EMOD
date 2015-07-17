/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "StandardEventCoordinator.h"

namespace Kernel
{
    class MultiInterventionEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, MultiInterventionEventCoordinator, IEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        MultiInterventionEventCoordinator();
        virtual void UpdateNodes(float dt);
        virtual bool visitIndividualCallback(IIndividualHumanEventContext *ihec, float &incrementalCostOut, ICampaignCostObserver * pICCO );

    protected:
        virtual void initializeInterventionConfig( const Configuration * inputJson );
        virtual void validateInterventionConfig( const json::Element& rElement );

    };
}
