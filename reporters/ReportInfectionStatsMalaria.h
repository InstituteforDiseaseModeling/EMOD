

#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"

namespace Kernel
{
    class ReportInfectionStatsMalaria: public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportInfectionStatsMalaria, IReport )
    public:
        ReportInfectionStatsMalaria();
        virtual ~ReportInfectionStatsMalaria();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;

    protected:
        ReportInfectionStatsMalaria( const std::string& rReportName );

        float m_StartDay;
        float m_EndDay;
        float m_ReportingInterval;
        float m_NextDayToCollectData;
        bool  m_IsCollectingData;
        bool  m_IncludeColumnHepatocytes;
        bool  m_IncludeColumnIRBC;
        bool  m_IncludeColumnGametocytes;
        float m_ThresholdHepatocytes;
        float m_ThresholdIRBC;
        float m_ThresholdGametocytes;
    };
}
