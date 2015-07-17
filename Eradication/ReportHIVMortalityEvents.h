/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "BaseTextReportEvents.h"
#include "Types.h"

namespace Kernel {
    struct ISimulation;

    class ReportHIVMortalityEvents : public BaseTextReportEvents
    {
    public:
        ReportHIVMortalityEvents( const ISimulation* );
        static IReport* ReportHIVMortalityEvents::Create( const ISimulation* parent ) { return new ReportHIVMortalityEvents( parent ); }

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual std::string GetHeader() const ;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange );

    private:
        const ISimulation * _parent;
    };
}

