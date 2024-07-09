

#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"

namespace Kernel
{
    class ReportDrugStatus: public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportDrugStatus, IReport )
    public:
        ReportDrugStatus();
        virtual ~ReportDrugStatus();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual std::string GetHeader() const override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;

    protected:
        ReportDrugStatus( const std::string& rReportName );

        float m_StartDay;
        float m_EndDay;
    };
}
