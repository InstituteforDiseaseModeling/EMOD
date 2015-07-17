/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Report.h"
#include "IIndividualHuman.h"
#include "Types.h"

namespace Kernel {

    class ReportSTI : public Report
    {
    public:
        ReportSTI();
        static IReport* ReportSTI::CreateReport() { return new ReportSTI(); }

        virtual void LogNodeData( INodeContext* pNC );
        virtual void LogIndividualData( IndividualHuman* individual );
        virtual void EndTimestep( float currentTime, float dt );

    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
        virtual void postProcessAccumulatedData();

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

        unsigned int num_predebut;
        unsigned int num_single_men;
        unsigned int num_single_women;
        unsigned int num_paired;

        unsigned int num_transitory;
        unsigned int num_informal;
        unsigned int num_marital;
        unsigned int num_sexually_active_prevalance;
        unsigned int num_post_debut_pop;
        NaturalNumber num_circumcised_males;
    };
}
