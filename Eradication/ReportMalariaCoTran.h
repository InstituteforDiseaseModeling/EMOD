
#pragma once

#include "ReportMalaria.h"
#include "BroadcasterObserver.h"

namespace Kernel {

    class ReportMalariaCoTran : public ReportMalaria
    {
    public:
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        ReportMalariaCoTran();
        virtual ~ReportMalariaCoTran() {};

        static IReport* CreateReport() { return new ReportMalariaCoTran(); }

        // ReportVector
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void BeginTimestep() override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;

        // IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *pHuman, const EventTrigger& trigger ) override;

    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
        virtual void postProcessAccumulatedData() override;

        bool m_IsRegisteredCoTran;

        ChannelID avg_num_cytokines_id;
        ChannelID num_transmissions_id;
        ChannelID avg_num_transmitted_id;
        ChannelID avg_num_acquired_id;
        ChannelID new_vector_infs_id;
        ChannelID avg_infs_in_new_vector_id;
        ChannelID infected_vectors_id;
        ChannelID infectious_vectors_id;
        ChannelID avg_infs_infected_id;
        ChannelID avg_infs_infectious_id;
    };
}
