
#pragma once

#include "ReportMalariaFiltered.h"
#include "ReportFactory.h"

namespace Kernel {

    class ReportMalariaFilteredIntraHost : public ReportMalariaFiltered
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportMalariaFilteredIntraHost, IReport )
        DECLARE_QUERY_INTERFACE()
    public:
        ReportMalariaFilteredIntraHost();
        virtual ~ReportMalariaFilteredIntraHost() {};

        // ReportVector
        virtual bool Configure( const Configuration* inputJson ) override;

    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;

        virtual void BeginTimestepOther() override;
        virtual void LogIndividualDataOther( IIndividualHuman* individual ) override;
        virtual void EndTimestepOther( float currentTime, float dt ) override;
        virtual void postProcessAccumulatedDataOther() override;
        virtual void RemoveUnwantedChannels() override;

        float m_InfectionDurationMax;

        ChannelID fraction_infected_id;
        ChannelID max_inf_pop_fraction_id;
        ChannelID max_inf_avg_duration_id;
        ChannelID inf_duration_max_id;
        ChannelID inf_fraction_stage_hepatocytes_id;
        ChannelID inf_fraction_stage_hepatocytes_new_id;
        ChannelID inf_fraction_stage_asexual_id;
        ChannelID inf_fraction_stage_asexual_new_id;
        ChannelID inf_fraction_stage_gametocytes_only_id;
        ChannelID inf_fraction_stage_gametocytes_only_new_id;
        ChannelID avg_cytokines_id;
        ChannelID msp_variant_fraction_id;
    };
}
