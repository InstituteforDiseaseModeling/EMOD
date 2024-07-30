
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

