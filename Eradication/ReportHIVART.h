/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>

#include "BaseTextReportEvents.h"
#include "ISimulation.h"

namespace Kernel
{
    class ReportHIVART : public BaseTextReportEvents
    {
    public:
        static IReport* Create( ISimulation* sim );

    protected:
        ReportHIVART( ISimulation* sim );
        virtual ~ReportHIVART();

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual std::string GetHeader() const ;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger );

    protected:

    private:
        ISimulation* simulation ;
    };
}
