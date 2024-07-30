
#pragma once

#include "ReportMalaria.h"
#include "BroadcasterObserver.h"

namespace Kernel {

    class ReportMalariaGenetics : public ReportMalaria
    {
    public:
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        ReportMalariaGenetics();
        virtual ~ReportMalariaGenetics() {};

        static IReport* CreateReport() { return new ReportMalariaGenetics(); }

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

        bool m_IsRegisteredGenetics;

        ChannelID people_hrp_deleted_id;
        ChannelID people_drug_resistant_id;
        ChannelID infs_hrp_deleted_id;
        ChannelID infs_drug_resistant_id;
        ChannelID avg_num_vector_inf_id;
        ChannelID new_vector_infs_id;
        ChannelID complexity_of_infection_id;
        ChannelID inf_vectors_id;
        ChannelID num_total_infections_id;
    };
}
