
#pragma once

#include <map>

#include "BaseEventReport.h"
#include "ChannelDataMap.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    class ReportEventCounter : public BaseEventReport
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportEventCounter, IReport )
#endif
    public:
        ReportEventCounter();
        virtual ~ReportEventCounter();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return BaseEventReport::AddRef(); }
        virtual int32_t Release() override { return BaseEventReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;
        virtual void BeginTimestep() override;
        virtual void Reduce() override;
        virtual void Finalize() override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override;
    private:
        ChannelDataMap channelDataMap ;
        std::map<std::string, std::string> unitsMap;
        std::vector<ChannelID> event_trigger_index_to_channel_id;
    };
}
