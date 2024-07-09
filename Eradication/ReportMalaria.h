
#pragma once

#include <deque>
#include "ReportVector.h"
#include "IReportMalariaDiagnostics.h"
#include "EventTrigger.h"
#include "BroadcasterObserver.h"

namespace Kernel {

    class ReportMalaria : public ReportVector
                        , public IReportMalariaDiagnostics
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportMalaria)
    public:
        // needed for IReportMalariaDiagnostics
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        ReportMalaria();
        virtual ~ReportMalaria() {};

        static IReport* CreateReport() { return new ReportMalaria(); }

        // ReportVector
        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void BeginTimestep() override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;

        // IReportMalariaDiagnostics
        virtual void SetDetectionThresholds( const std::vector<float>& rDetectionThresholds ) override;

        // IObserver
        bool notifyOnEvent( IIndividualHumanEventContext *pEntity, const EventTrigger& trigger ) override;

    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
        virtual void postProcessAccumulatedData() override;

        virtual const char* GetParameterNameFor30DayAvg() const;
        virtual const char* GetDescTextFor30DayAvg() const;
        virtual const char* GetDependsOnFor30DayAvg() const;

        ChannelID num_people_infected_id;
        ChannelID avg_num_inf_id;
        ChannelID avg_infection_dur_id;
        ChannelID major_variant_fraction_id;
        ChannelID mean_parasitemia_id;
        ChannelID new_clinical_cases_id;
        ChannelID new_severe_cases_id;

        std::vector<ChannelID> m_PrevalenceByDiagnosticChannelIDs;

        std::vector<float> m_DetectionThresholds;
        std::vector<float> m_Detected;
        float              m_MeanParasitemia;
        std::deque<uint32_t> m_NumInfectionsClearedQueue; //last 30 days
        std::deque<float>    m_InfectionClearedDurationQueue;
        bool m_Include30DayAvg;
    };
}
