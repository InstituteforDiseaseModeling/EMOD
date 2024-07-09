
#pragma once

#include <map>
#include "ReportSTI.h"
#include "IndividualEventContext.h"
#include "Interventions.h"

namespace Kernel 
{
    class ReportHIV : public ReportSTI
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportHIV)
    public:
        ReportHIV();
        virtual ~ReportHIV();

        static IReport* ReportHIV::CreateReport() { return new ReportHIV(); }

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;

        // for IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override;

        // ISupports

    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
        virtual void postProcessAccumulatedData() override;

        ChannelID num_hiv_acute_id;
        ChannelID num_hiv_latent_id;
        ChannelID num_hiv_untreated_aids_id;
        ChannelID num_hiv_treated_aids_id;
        ChannelID num_hiv_cd4_hi_on_ART_id;
        ChannelID num_hiv_cd4_hi_non_ART_id;
        ChannelID num_hiv_cd4_lo_on_ART_id;
        ChannelID num_hiv_cd4_lo_non_ART_id;
        ChannelID num_on_ART_id;
        ChannelID num_ART_dropouts_id;
        ChannelID num_events_id;
        std::vector<ChannelID> event_trigger_index_to_channel_id;

        float num_acute;
        float num_latent;
        float num_aids_without;
        float num_aids_with;
        float num_hiv_cd4_lo_non_ART;
        float num_hiv_cd4_hi_non_ART;
        float num_hiv_cd4_lo_on_ART;
        float num_hiv_cd4_hi_on_ART;
        float num_on_ART;
        float num_ART_dropouts;
        unsigned int num_events ;
        bool counting_all_events;
        std::vector<uint32_t> event_counter_vector; // indexed by EventTrigger index
        std::vector< EventTrigger > eventTriggerList ;
    };
}
