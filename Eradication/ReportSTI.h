
#pragma once

#include "Report.h"
#include "IIndividualHuman.h"
#include "IRelationship.h"
#include "Types.h"

namespace Kernel {

    class ReportSTI : public Report
    {
    public:
        ReportSTI();
        static IReport* ReportSTI::CreateReport() { return new ReportSTI(); }

        virtual void Initialize( unsigned int nrmSize ) override;
        virtual bool Configure( const Configuration* config ) override;

        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual void LogNodeData( INodeContext* pNC ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;

        // for IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override;
    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
        virtual void postProcessAccumulatedData() override;

        bool m_IncludeCoitalActs;

        ChannelID sexually_active_prevalence_id;
        ChannelID sexually_active_universe_id;
        ChannelID single_men_id;
        ChannelID single_women_id;
        ChannelID paired_people_id;
        ChannelID ymi_id;
        ChannelID yfi_id;
        ChannelID pdi_id;
        ChannelID num_circ_males_id;
        ChannelID ymc_id;
        ChannelID yfc_id;
        ChannelID outside_pfa_id;
        ChannelID infs_outside_pfa_id;
        std::vector<ChannelID> active_rel_ids;
        std::vector<ChannelID> acts_this_dt_rel_ids;
        std::vector<ChannelID> acts_this_dt_condoms_rel_ids;

        unsigned int num_marrieds;
        unsigned int num_singles;
        unsigned int sexually_mature_pop;
        unsigned int sexually_active_adults;
        unsigned int concurrents;
        unsigned int single_women_in_concurrents;
        unsigned int num_discordant_rels;
        unsigned int num_concordant_pos_rels;
        unsigned int num_concordant_neg_rels;
        NaturalNumber num_sexually_active_universe;

        unsigned int num_single_men;
        unsigned int num_single_women;
        unsigned int num_paired;

        unsigned int num_rels[ RelationshipType::COUNT ];
        unsigned int num_acts[ RelationshipType::COUNT ];
        unsigned int num_acts_condoms[ RelationshipType::COUNT ];
        unsigned int num_outside_pfa;
        unsigned int num_infections_outside_pfa;

        unsigned int num_sexually_active_prevalance;
        unsigned int num_post_debut_pop;
        NaturalNumber num_circumcised_males;

        NonNegativeFloat youngMaleInfected;
        NonNegativeFloat youngMaleCount;
        NonNegativeFloat youngFemaleInfected;
        NonNegativeFloat youngFemaleCount;


    };
}
