/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
    typedef std::vector<PfPRbinned_t> Infectionbinned_t;

    class  INodeVector;
    struct IIndividualHumanEventContext;
    struct IJsonObjectAdapter;
    class JSerializer;


    class ReportIntervalData : public IIntervalData
    {
    public:
        ReportIntervalData();
        virtual ~ReportIntervalData();

        // IIntervalData methods
        virtual void Clear() override;
        virtual void Update(const IIntervalData& rOther) override;
        virtual void Serialize(IJsonObjectAdapter& rjoa, JSerializer& js) override;
        virtual void Deserialize(IJsonObjectAdapter& rjoa) override;

        // other methods
        void SetVectorSize(int age_size, int PfPR_size, int Infectiousness_size);

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
        PfPRbinned_t sum_binned_PfPR_by_agebin_smeared;
        PfPRbinned_t sum_binned_PfgamPR_by_agebin_smeared;
        PfPRbinned_t sum_binned_PfPR_by_agebin_true_smeared;
        PfPRbinned_t sum_binned_PfgamPR_by_agebin_true_smeared;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin_age_scaled;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin_smeared;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam;
        Infectionbinned_t sum_binned_infection_by_pfprbin_and_agebin_smeared_inf_and_gam_age_scaled;
        double sum_no_infected_days;
        double sum_days_under_1pct_infected;
    };


    class MalariaSummaryReport : public BaseEventReportIntervalOutput
    {
    public:
        MalariaSummaryReport();
        virtual ~MalariaSummaryReport();

        // BaseEventReportIntervalOutput
        virtual bool Configure(const Configuration* config) override;
        virtual bool notifyOnEvent(IIndividualHumanEventContext *context, const EventTrigger& trigger) override;
        virtual void EndTimestep(float currentTime, float dt) override;

    protected:
        // BaseEventReportIntervalOutput
        virtual void AccumulateOutput() override;
        virtual void SerializeOutput(float currentTime, IJsonObjectAdapter& output, JSerializer& js) override;

        INodeVector* node_vector;
        std::vector<float> ages;
        std::vector<float> PfPRbins;
        std::vector<float> Infectionbins;

        // accumulated on each timestep, reset on reporting interval
        ReportIntervalData* m_pReportData;

        // accumulated on each reporting interval, written to output
        std::vector<float> time_of_report;
        std::vector<double> annual_EIRs;
        std::vector<double> PfPRs_2to10;
        std::vector<agebinned_t> PfPRs_by_agebin;
        std::vector<agebinned_t> PfgamPRs_by_agebin;
        std::vector<agebinned_t> mean_log_parasite_density_by_agebin;
        std::vector<PfPRbinned_t> binned_PfPRs_by_agebin;
        std::vector<PfPRbinned_t> binned_PfgamPRs_by_agebin;
        std::vector<PfPRbinned_t> binned_PfPRs_by_agebin_smeared;
        std::vector<PfPRbinned_t> binned_PfgamPRs_by_agebin_smeared;
        std::vector<PfPRbinned_t> binned_PfPRs_by_agebin_true_smeared;
        std::vector<PfPRbinned_t> binned_PfgamPRs_by_agebin_true_smeared;
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
        std::vector<Infectionbinned_t> binned_Infectiousness;
        std::vector<Infectionbinned_t> binned_Infectiousness_age_scaled;
        std::vector<Infectionbinned_t> binned_Infectiousness_smeared;
        std::vector<Infectionbinned_t> binned_Infectiousness_smeared_inf_and_gam;
        std::vector<Infectionbinned_t> binned_Infectiousness_smeared_inf_and_gam_age_scaled;

        std::vector<double> duration_no_infection_streak;
        std::vector<double> fraction_under_1pct_infected;
    };
}