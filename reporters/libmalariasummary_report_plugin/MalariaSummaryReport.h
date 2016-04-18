/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include <map>

#include "BaseEventReportIntervalOutput.h"

namespace Kernel
{
    typedef std::vector<double> agebinned_t;
    typedef std::vector<agebinned_t> PfPRbinned_t;

    class  INodeVector;
    struct IIndividualHumanEventContext;
    class MalariaSummaryReport : public BaseEventReportIntervalOutput
    {
    public:
        MalariaSummaryReport();
        virtual ~MalariaSummaryReport();

        // BaseEventReportIntervalOutput
        virtual bool Configure( const Configuration* config ) override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void Finalize() override;

    protected:
        // BaseEventReportIntervalOutput
        void WriteOutput( float currentTime );

        void AccumulateOutput();
        int  GetPfPRBin(float parasite_count);

        bool m_has_data ;
        INodeVector* node_vector;
        bool expired ;

        // accumulated on each timestep, reset on reporting interval
        std::vector<float> ages;
        double sum_EIR;
        double sum_population_2to10;
        double sum_parasite_positive_2to10;
        agebinned_t sum_population_by_agebin;
        agebinned_t sum_parasite_positive_by_agebin;
        agebinned_t sum_gametocyte_positive_by_agebin;
        agebinned_t sum_log_parasite_density_by_agebin;
        agebinned_t sum_rdt_positive_by_agebin;
        agebinned_t sum_clinical_cases_by_agebin;
        agebinned_t sum_severe_cases_by_agebin;
        agebinned_t sum_severe_anemia_by_agebin;
        agebinned_t sum_moderate_anemia_by_agebin;
        agebinned_t sum_mild_anemia_by_agebin;
        agebinned_t sum_severe_cases_by_anemia_by_agebin;
        agebinned_t sum_severe_cases_by_parasites_by_agebin;
        agebinned_t sum_severe_cases_by_fever_by_agebin;
        PfPRbinned_t sum_binned_PfPR_by_agebin;
		PfPRbinned_t sum_binned_PfgamPR_by_agebin;
		double sum_no_infected_days;
        double sum_days_under_1pct_infected;

        // accumulated on each reporting interval, written to output
        std::vector<double> annual_EIRs;
        std::vector<double> PfPRs_2to10;
        std::vector<agebinned_t> PfPRs_by_agebin;
        std::vector<agebinned_t> PfgamPRs_by_agebin;
        std::vector<agebinned_t> mean_log_parasite_density_by_agebin;
        std::vector<PfPRbinned_t> binned_PfPRs_by_agebin;
        std::vector<PfPRbinned_t> binned_PfgamPRs_by_agebin;
        std::vector<agebinned_t> RDT_PfPRs_by_agebin;
        std::vector<agebinned_t> annual_clinical_incidences_by_agebin;
        std::vector<agebinned_t> annual_severe_incidences_by_agebin;
        std::vector<agebinned_t> average_population_by_agebin;
        std::vector<agebinned_t> annual_severe_incidences_by_anemia_by_agebin;
        std::vector<agebinned_t> annual_severe_incidences_by_parasites_by_agebin;
        std::vector<agebinned_t> annual_severe_incidences_by_fever_by_agebin;
        std::vector<agebinned_t> annual_severe_anemia_by_agebin;
        std::vector<agebinned_t> annual_moderate_anemia_by_agebin;
        std::vector<agebinned_t> annual_mild_anemia_by_agebin;

        std::vector<double> duration_no_infection_streak;
        std::vector<double> fraction_under_1pct_infected;
    };
}