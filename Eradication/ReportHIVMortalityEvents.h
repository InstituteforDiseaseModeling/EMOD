/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseTextReportEvents.h"
#include "Types.h"
#include "Properties.h"

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
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger ) override;

    private:
        const ISimulation * _parent;
        IPKey m_InterventionStatusKey;
    };
}

