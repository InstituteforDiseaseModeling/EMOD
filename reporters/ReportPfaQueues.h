
#pragma once

#include "BaseTextReport.h"
#include "ReportFactory.h"

namespace Kernel
{
    struct ISociety;

    class ReportPfaQueues : public BaseTextReport
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportPfaQueues, IReport )
    public:
        ReportPfaQueues();
        virtual ~ReportPfaQueues();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual std::string GetHeader() const override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

    protected:
        ReportPfaQueues( const std::string& rReportName );

        std::string GetHeader( ISociety* pSociety ) const;

        bool m_AddedHeader;
    };
}
