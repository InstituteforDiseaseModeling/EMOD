
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
