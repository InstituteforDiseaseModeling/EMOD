/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "StandardEventCoordinator.h"

namespace Kernel
{
    class CalendarEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, CalendarEventCoordinator, IEventCoordinator)
        DECLARE_CONFIGURED(CalendarEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        CalendarEventCoordinator();
        virtual void UpdateNodes(float dt) override;
        virtual bool TargetedIndividualIsCovered( IIndividualHumanEventContext *ihec ) override;

    protected:
        void BuildDistributionCalendar( std::vector<int>, std::vector<float> );
        virtual void InitializeRepetitions( const Configuration* inputJson ) override;
        virtual bool IsTimeToUpdate( float dt ) override;
        virtual void UpdateRepetitions() override;

        std::map< NaturalNumber, Fraction > times_and_coverages;
    };
}
