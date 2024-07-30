
#pragma once
#include "BaseTextReport.h"
#include "IIndividualHuman.h"

namespace Kernel
{
    struct INodeSTI;
    struct ISociety;

    class StiRelationshipQueueReporter : public BaseTextReport
    {
    public:
        StiRelationshipQueueReporter();
        virtual ~StiRelationshipQueueReporter();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseTextReport::AddRef(); }
        virtual int32_t Release() override { return BaseTextReport::Release(); }

        // IReport
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void LogNodeData( INodeContext* pNC ) override;

    protected:
        // BaseTextReport
        virtual std::string GetHeader() const override;

    private:
        std::string CreateLengthsString( const map<int, vector<int>>& rQueueLengthMap );
        std::string CreateBinString( const std::vector<int>& rQueue, int* pTotalCount );

        bool m_FirstTime ;
    };
}