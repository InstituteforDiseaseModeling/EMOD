/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>

#include "ReportEventRecorderCoordinator.h"
#include "ReportStatsByIP.h"

namespace Kernel
{
    class ReportSurveillanceEventRecorder : public ReportEventRecorderCoordinator
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER( ReportSurveillanceEventRecorder )

    public:
        static IReport* CreateReport();

        static std::string GetEnableParameterName(); // hides base class implementation

    public:
        ReportSurveillanceEventRecorder();
        virtual ~ReportSurveillanceEventRecorder();

        // ------------
        // --- IReport
        // ------------
        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;

        virtual bool notifyOnEvent( IEventCoordinatorEventContext *pEntity, const EventTriggerCoordinator& trigger ) override;

    protected:
        virtual std::string GetOtherData( IEventCoordinatorEventContext *context, const EventTriggerCoordinator& trigger ) override;

        jsonConfigurable::tDynamicStringSet m_StatsByIpKeyNames;
        ReportStatsByIP m_ReportStatsByIP;
    };
}
