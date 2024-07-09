
#pragma once

#include <vector>

#include "BaseTextReportEvents.h"
#include "ReportFactory.h"
#include "IReportMalariaDiagnostics.h"
#include "ReportFilter.h"

namespace Kernel
{
    class ReportSimpleMalariaTransmission : public BaseTextReportEvents
                                          , public IReportMalariaDiagnostics
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportSimpleMalariaTransmission, IReport )
    public:
        ReportSimpleMalariaTransmission();
        virtual ~ReportSimpleMalariaTransmission();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReportEvents::AddRef(); }
        virtual int32_t Release() override { return BaseTextReportEvents::Release(); }

        // BaseTextReportEvents
        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& nodeIds_demographics ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool IsValidNode( INodeEventContext* pNEC ) const override;
        virtual std::string GetHeader() const override;
        virtual std::string GetReportName() const override;

        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger) override;

        // IReportMalariaDiagnostics
        virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

    protected:

        ReportFilter m_ReportFilter;
        bool m_OutputWritten;
        std::vector<float> m_DetectionThresholds;
    };
}