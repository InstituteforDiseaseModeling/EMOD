/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "StdAfx.h"
#include "MalariaSummaryReport.h"

#include <algorithm>

#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IndividualMalaria.h"
#include "VectorContexts.h"
#include "VectorPopulation.h"
#include "SusceptibilityMalaria.h"
#include "NodeEventContext.h"
#include "ReportUtilities.h"

#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Module name for logging, CustomReport.json, and DLL GetType()
static const char * _module = "MalariaSummaryReport";  // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", nullptr}; // <<< Types of simulation the report is to be used with

Kernel::report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new MalariaSummaryReport()); // <<< Report to create
};

DllInterfaceHelper DLL_HELPER( _module, _sim_types, rif );

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// ------------------------------
// --- DLL Interface Methods
// ---
// --- The DTK will use these methods to establish communication with the DLL.
// ------------------------------

#ifdef __cplusplus    // If used by C++ code, 
extern "C" {          // we need to export the C interface
#endif

DTK_DLLEXPORT char* __cdecl
GetEModuleVersion(char* sVer, const Environment * pEnv)
{
    return DLL_HELPER.GetEModuleVersion( sVer, pEnv );
}

DTK_DLLEXPORT void __cdecl
GetSupportedSimTypes(char* simTypes[])
{
    DLL_HELPER.GetSupportedSimTypes( simTypes );
}

DTK_DLLEXPORT const char * __cdecl
GetType()
{
    return DLL_HELPER.GetType();
}

DTK_DLLEXPORT void __cdecl
GetReportInstantiator( Kernel::report_instantiator_function_t* pif )
{
    DLL_HELPER.GetReportInstantiator( pif );
}

#ifdef __cplusplus
}
#endif

// ----------------------------------------
// --- MalariaSummaryReport Methods
// ----------------------------------------

    float PfPRbinedges[] = { 50, 500, 5000, 50000, FLT_MAX };
    static const std::vector<float> PfPRbins = std::vector<float>(PfPRbinedges, PfPRbinedges + sizeof(PfPRbinedges)/sizeof(float));

    MalariaSummaryReport::MalariaSummaryReport() 
        : BaseEventReportIntervalOutput( _module ) 
        , node_vector(nullptr)
        , expired(false)
        , ages()
        , sum_EIR(0)
        , sum_population_2to10(0)
        , sum_parasite_positive_2to10(0)
        , sum_binned_PfPR_by_agebin()
        , sum_binned_PfgamPR_by_agebin()
        , sum_no_infected_days(0)
        , sum_days_under_1pct_infected(0)
    {
    }

    bool MalariaSummaryReport::Configure( const Configuration * inputJson )
    {
        if( inputJson->Exist("Age_Bins") )
        {
            initConfigTypeMap("Age_Bins", &ages, "Age Bins (in years) to aggregate within and report");
        }
        else
        {
            ages.push_back(    0.0f );
            ages.push_back(   10.0f );
            ages.push_back(   20.0f );
            ages.push_back(   30.0f );
            ages.push_back(   40.0f );
            ages.push_back(   50.0f );
            ages.push_back(   60.0f );
            ages.push_back(   70.0f );
            ages.push_back(   80.0f );
            ages.push_back(   90.0f );
            ages.push_back(  100.0f );
            ages.push_back( 1000.0f );
        }

        bool configured = BaseEventReportIntervalOutput::Configure( inputJson );
 
        sum_population_by_agebin.resize(                ages.size(), 0);
        sum_log_parasite_density_by_agebin.resize(      ages.size(), 0);
        sum_parasite_positive_by_agebin.resize(         ages.size(), 0);
        sum_gametocyte_positive_by_agebin.resize(       ages.size(), 0);
        sum_rdt_positive_by_agebin.resize(              ages.size(), 0);
        sum_clinical_cases_by_agebin.resize(            ages.size(), 0);
        sum_severe_cases_by_agebin.resize(              ages.size(), 0);
        sum_severe_anemia_by_agebin.resize(             ages.size(), 0);
        sum_moderate_anemia_by_agebin.resize(           ages.size(), 0);
        sum_mild_anemia_by_agebin.resize(               ages.size(), 0);
        sum_severe_cases_by_anemia_by_agebin.resize(    ages.size(), 0);
        sum_severe_cases_by_parasites_by_agebin.resize( ages.size(), 0);
        sum_severe_cases_by_fever_by_agebin.resize(     ages.size(), 0);

        for (int j = 0; j<PfPRbins.size(); j++)
        {
            sum_binned_PfPR_by_agebin.push_back(agebinned_t(ages.size()));
			sum_binned_PfgamPR_by_agebin.push_back(agebinned_t(ages.size()));
        }
 
        return configured;
    }

    MalariaSummaryReport::~MalariaSummaryReport()
    {
    }

    void MalariaSummaryReport::Finalize()
    {
        if(!expired && m_has_data )
        {
            LOG_WARN("Summary report not written yet, but the simulation is over.  Writing now...\n");
            AccumulateOutput();
            WriteOutput( -999.0 );
        }
    }

    void MalariaSummaryReport::EndTimestep( float currentTime, float dt )
    {
        if( !HaveRegisteredAllEvents() || HaveUnregisteredAllEvents() )
        {
            // --------------------------------------------------------------------------------
            // --- If we have either not registered or unregistered listening for events, then
            // --- we don't want to consider outputing data.
            // --------------------------------------------------------------------------------
            return;
        }

        // Cache an INodeVector pointer
        INodeEventContext* p_nec = GetFirstINodeEventContext();
        release_assert( p_nec );
        if  (!node_vector)
        {
            if( p_nec->GetNodeContext()->QueryInterface( GET_IID( INodeVector ), (void**) & node_vector ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "p_nec->GetNodeContext()", "INodeVector", "INodeContext" );
            }
        }
        m_has_data = true ;

        std::list<VectorPopulation*> vectorPopulations = node_vector->GetVectorPopulations();
        for( auto vectorpopulation : vectorPopulations )
        {
            sum_EIR += vectorpopulation->GetEIRByPool(VectorPoolIdEnum::BOTH_VECTOR_POOLS);
        }

        // add to streak or reset to zero days of no infections
        INodeContext* context = p_nec->GetNodeContext();
        if (context->GetInfected() == 0 )
        {
            sum_no_infected_days += 1;
        }
        else
        {
            sum_no_infected_days = 0;
        }

        if ( ((double)context->GetInfected() / (double)context->GetStatPop()) < 0.01)
        {
            sum_days_under_1pct_infected += 1;
        }

        m_interval_timer+=dt;
        if ( !expired && m_interval_timer >= m_reporting_interval )
        {
            AccumulateOutput();

            LOG_DEBUG("Resetting MalariaSummaryReport reporting interval timer...\n");
            m_interval_timer -= m_reporting_interval; // allows for dealing with reporting intervals that are not whole days, for example 12 approximately equal reports in a year = 30.42 day interval
            m_report_count++;

            if ( m_report_count >= m_max_number_reports)
            {
                WriteOutput( currentTime );
                expired = true; // stop writing output
                UnregisterAllNodes();
            }
        }
    }

    bool MalariaSummaryReport::notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange )
    {
        LOG_DEBUG_F( "MalariaSummaryReport notified of event by %d-year old individual.\n", (int) (context->GetAge() / DAYSPERYEAR) );

        // individual context for suid
        IIndividualHumanContext * iindividual = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanContext), (void**)&iindividual) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanContext", "IIndividualHumanEventContext");
        }
        m_has_data = true ;

        int id           = iindividual->GetSuid().data;
        double mc_weight = context->GetMonteCarloWeight();
        double age       = context->GetAge();
        int agebin       = ReportUtilities::GetAgeBin( (float)age, ages );

        bool is2to10 = false;
        if ( age > 2*DAYSPERYEAR && age < 10*DAYSPERYEAR )
        {
            is2to10 = true;
            sum_population_2to10 += mc_weight;
        }
        sum_population_by_agebin.at(agebin) += mc_weight;

        // get malaria contexts
        IMalariaHumanContext * individual_malaria = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&individual_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IMalariaHumanContext", "IIndividualHumanEventContext");
        }
        IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();

        // push back today's disease variables for infected individuals
        if ( context->IsInfected() )
        {
            float parasite_count = individual_malaria->CheckParasiteCountWithTest( MALARIA_TEST_BLOOD_SMEAR );
            if (parasite_count > 0)
            {
                if(is2to10)
                {
                    sum_parasite_positive_2to10 += mc_weight;
                }
                sum_parasite_positive_by_agebin.at(agebin) += mc_weight;
                if ( !( _isnanf ( log10(parasite_count) ) ) && _finitef(log10(parasite_count)) ) 
                {
                    sum_log_parasite_density_by_agebin.at(agebin) += log10(parasite_count);
                }
            }
            int PfPRbin = GetPfPRBin(susceptibility_malaria->get_parasite_density());
            sum_binned_PfPR_by_agebin.at(PfPRbin).at(agebin) += mc_weight;
			float gametocyte_count = individual_malaria->GetGametocyteDensity();
            int PfgamPRbin = GetPfPRBin(gametocyte_count);
            sum_binned_PfgamPR_by_agebin.at(PfgamPRbin).at(agebin) += mc_weight;
			gametocyte_count = individual_malaria->CheckGametocyteCountWithTest(MALARIA_TEST_BLOOD_SMEAR);
			if (gametocyte_count > 0.02)
			{
	            sum_gametocyte_positive_by_agebin.at(agebin) += mc_weight;
			}

            
            float hemoglobin = susceptibility_malaria->GetHemoglobin();
            if ( hemoglobin < 5 )  { sum_severe_anemia_by_agebin.at(agebin)   += mc_weight; }
            if ( hemoglobin < 8 )  { sum_moderate_anemia_by_agebin.at(agebin) += mc_weight; }
            if ( hemoglobin < 11 ) { sum_mild_anemia_by_agebin.at(agebin)     += mc_weight; }
            
            if (individual_malaria->CheckForParasitesWithTest(MALARIA_TEST_NEW_DIAGNOSTIC))
            {
                sum_rdt_positive_by_agebin.at(agebin) += mc_weight;
            }

            if (individual_malaria->HasClinicalSymptom(ClinicalSymptomsEnum::CLINICAL_DISEASE))
            {
                sum_clinical_cases_by_agebin.at(agebin) += mc_weight;
            }
            if (individual_malaria->HasClinicalSymptom(ClinicalSymptomsEnum::SEVERE_DISEASE))
            {
                sum_severe_cases_by_agebin.at(agebin) += mc_weight;
                switch( susceptibility_malaria->CheckSevereCaseType() )
                {
                    case Kernel::SevereCaseTypesEnum::ANEMIA:
                        sum_severe_cases_by_anemia_by_agebin.at(agebin) += mc_weight;
                    break;

                    case Kernel::SevereCaseTypesEnum::PARASITES:
                        sum_severe_cases_by_parasites_by_agebin.at(agebin) += mc_weight;
                    break;

                    case Kernel::SevereCaseTypesEnum::FEVER:
                        sum_severe_cases_by_fever_by_agebin.at(agebin) += mc_weight;
                    break;
                }
            }

        }
        else
        {

        }

        return true;
    }

    int MalariaSummaryReport::GetPfPRBin(float parasite_count)
    {
        vector<float>::const_iterator it;
        it = std::lower_bound(PfPRbins.begin(), PfPRbins.end(), parasite_count);
        int PfPRbin_idx = it - PfPRbins.begin();
        return PfPRbin_idx;
    }

    void MalariaSummaryReport::AccumulateOutput()
    {
        LOG_INFO_F("Aggregating output on reporting interval of %d days\n", (int)m_reporting_interval);
        annual_EIRs.push_back( 365.0 * sum_EIR / m_reporting_interval );

        if(sum_population_2to10 > 0)
        {
            PfPRs_2to10.push_back( sum_parasite_positive_2to10 / sum_population_2to10 );
        }
        else
        {
            PfPRs_2to10.push_back(0.0);
        }
        agebinned_t pfprs;
        agebinned_t pfgamprs;
        agebinned_t log_parasite_density;
        agebinned_t rdt_pfprs;
        agebinned_t annual_clinical_incidences;
        agebinned_t annual_severe_incidences;
        agebinned_t average_population;
        agebinned_t annual_severe_incidences_by_anemia;
        agebinned_t annual_severe_incidences_by_parasites;
        agebinned_t annual_severe_incidences_by_fever;
        agebinned_t annual_severe_anemia;
        agebinned_t annual_moderate_anemia;
        agebinned_t annual_mild_anemia;
        PfPRbinned_t pfprs_binned;
        PfPRbinned_t pfgamprs_binned;
        for (int j = 0; j<PfPRbins.size(); j++)
        {
            pfprs_binned.push_back(agebinned_t(ages.size()));
            pfgamprs_binned.push_back(agebinned_t(ages.size()));
        }
        for(int i = 0; i<ages.size(); i++)
        {
            average_population.push_back(sum_population_by_agebin.at(i) * 1.0/m_reporting_interval);
            if (sum_population_by_agebin.at(i) > 0)
            {
                pfprs.push_back(sum_parasite_positive_by_agebin.at(i) / sum_population_by_agebin.at(i));
                pfgamprs.push_back(sum_gametocyte_positive_by_agebin.at(i) / sum_population_by_agebin.at(i));
                rdt_pfprs.push_back(sum_rdt_positive_by_agebin.at(i) / sum_population_by_agebin.at(i));
                annual_clinical_incidences.push_back(365.0 * sum_clinical_cases_by_agebin.at(i) / sum_population_by_agebin.at(i));
                annual_severe_incidences.push_back(365.0 * sum_severe_cases_by_agebin.at(i) / sum_population_by_agebin.at(i));
                annual_severe_incidences_by_anemia.push_back(365.0 * sum_severe_cases_by_anemia_by_agebin.at(i) / sum_population_by_agebin.at(i));
                annual_severe_incidences_by_parasites.push_back(365.0 * sum_severe_cases_by_parasites_by_agebin.at(i) / sum_population_by_agebin.at(i));
                annual_severe_incidences_by_fever.push_back(365.0 * sum_severe_cases_by_fever_by_agebin.at(i) / sum_population_by_agebin.at(i));
                annual_severe_anemia.push_back(365.0 * sum_severe_anemia_by_agebin.at(i) / sum_population_by_agebin.at(i));
                annual_moderate_anemia.push_back(365.0 * sum_moderate_anemia_by_agebin.at(i) / sum_population_by_agebin.at(i));
                annual_mild_anemia.push_back(365.0 * sum_mild_anemia_by_agebin.at(i) / sum_population_by_agebin.at(i));
                if (sum_parasite_positive_by_agebin.at(i) > 0)
                {
                    log_parasite_density.push_back(sum_log_parasite_density_by_agebin.at(i) / sum_parasite_positive_by_agebin.at(i));
                }
                else
                {
                    log_parasite_density.push_back(0.0);
                }
                for(int j = 0; j<PfPRbins.size(); j++)
                {
                    pfprs_binned.at(j).at(i) = sum_binned_PfPR_by_agebin.at(j).at(i) / sum_population_by_agebin.at(i);
                    pfgamprs_binned.at(j).at(i) = sum_binned_PfgamPR_by_agebin.at(j).at(i) / sum_population_by_agebin.at(i);
                }
            }
            else 
            {
                pfprs.push_back(0.0);
                pfgamprs.push_back(0.0);
                rdt_pfprs.push_back(0.0);
                log_parasite_density.push_back(0.0);
                annual_clinical_incidences.push_back(0.0);
                annual_severe_incidences.push_back(0.0);
                annual_severe_incidences_by_anemia.push_back(0.0);
                annual_severe_incidences_by_parasites.push_back(0.0);
                annual_severe_incidences_by_fever.push_back(0.0);
                annual_severe_anemia.push_back(0.0);
                annual_moderate_anemia.push_back(0.0);
                annual_mild_anemia.push_back(0.0);
                for(int j = 0; j<PfPRbins.size(); j++)
                {
                    pfprs_binned.at(j).push_back(0.0);
                    pfgamprs_binned.at(j).push_back(0.0);
                }
            }
        }
        PfPRs_by_agebin.push_back(pfprs);
        PfgamPRs_by_agebin.push_back(pfprs);
        RDT_PfPRs_by_agebin.push_back(rdt_pfprs);
        mean_log_parasite_density_by_agebin.push_back(log_parasite_density);
        annual_clinical_incidences_by_agebin.push_back(annual_clinical_incidences);
        annual_severe_incidences_by_agebin.push_back(annual_severe_incidences);
        average_population_by_agebin.push_back(average_population);
        annual_severe_incidences_by_anemia_by_agebin.push_back(annual_severe_incidences_by_anemia);
        annual_severe_incidences_by_parasites_by_agebin.push_back(annual_severe_incidences_by_parasites);
        annual_severe_incidences_by_fever_by_agebin.push_back(annual_severe_incidences_by_fever);
        annual_severe_anemia_by_agebin.push_back(annual_severe_anemia);
        annual_moderate_anemia_by_agebin.push_back(annual_moderate_anemia);
        annual_mild_anemia_by_agebin.push_back(annual_mild_anemia);
        binned_PfPRs_by_agebin.push_back(pfprs_binned);
        binned_PfgamPRs_by_agebin.push_back(pfgamprs_binned);

        duration_no_infection_streak.push_back(sum_no_infected_days);
        fraction_under_1pct_infected.push_back(sum_days_under_1pct_infected / m_reporting_interval);

        sum_EIR = 0;
        sum_population_2to10 = 0;
        sum_parasite_positive_2to10 = 0;

        std::fill(sum_population_by_agebin.begin(), sum_population_by_agebin.end(), 0);
        std::fill(sum_parasite_positive_by_agebin.begin(), sum_parasite_positive_by_agebin.end(), 0);
        std::fill(sum_gametocyte_positive_by_agebin.begin(), sum_gametocyte_positive_by_agebin.end(), 0);
        std::fill(sum_log_parasite_density_by_agebin.begin(), sum_log_parasite_density_by_agebin.end(), 0);
        std::fill(sum_rdt_positive_by_agebin.begin(), sum_rdt_positive_by_agebin.end(), 0);
        std::fill(sum_clinical_cases_by_agebin.begin(), sum_clinical_cases_by_agebin.end(), 0);
        std::fill(sum_severe_cases_by_agebin.begin(), sum_severe_cases_by_agebin.end(), 0);
        std::fill(sum_severe_anemia_by_agebin.begin(), sum_severe_anemia_by_agebin.end(), 0);
        std::fill(sum_moderate_anemia_by_agebin.begin(), sum_moderate_anemia_by_agebin.end(), 0);
        std::fill(sum_mild_anemia_by_agebin.begin(), sum_mild_anemia_by_agebin.end(), 0);
        std::fill(sum_severe_cases_by_anemia_by_agebin.begin(), sum_severe_cases_by_anemia_by_agebin.end(), 0);
        std::fill(sum_severe_cases_by_parasites_by_agebin.begin(), sum_severe_cases_by_parasites_by_agebin.end(), 0);
        std::fill(sum_severe_cases_by_fever_by_agebin.begin(), sum_severe_cases_by_fever_by_agebin.end(), 0);

        for(int j = 0; j<PfPRbins.size(); j++)
        {
            std::fill(sum_binned_PfPR_by_agebin.at(j).begin(), sum_binned_PfPR_by_agebin.at(j).end(), 0);
            std::fill(sum_binned_PfgamPR_by_agebin.at(j).begin(), sum_binned_PfgamPR_by_agebin.at(j).end(), 0);
        }
        // N.B. don't reset running count of no-infection streak here
        // sum_no_infected_days reset if GetInfected() > 0 in Update()
        sum_days_under_1pct_infected = 0;

    }

    void MalariaSummaryReport::WriteOutput( float currentTime )
    {
        std::stringstream output_file_name;
        output_file_name << GetBaseOutputFilename() << ".json";
        LOG_INFO_F( "Writing file: %s\n", output_file_name.str().c_str() );

        Element elementRoot = String();
        QuickBuilder qb(elementRoot);

        for (int i = 0; i < ages.size(); i++)
        {
            qb["Age Bins"][i] = Number(ages.at(i));
        }

        for (int j = 0; j < PfPRbins.size(); j++)
        {
            qb["Parasitemia Bins"][j] = Number(PfPRbins.at(j));
            qb["Gametocytemia Bins"][j] = Number(PfPRbins.at(j));
        }

        for (int n = 0; n < annual_EIRs.size(); n++)
        {
            qb["Annual EIR"][n] = Number(annual_EIRs.at(n));
            qb["PfPR_2to10"][n] = Number(PfPRs_2to10.at(n));
            qb["No Infection Streak"][n] = Number(duration_no_infection_streak.at(n));
            qb["Fraction Days Under 1pct Infected"][n] = Number(fraction_under_1pct_infected.at(n));

            for (int i = 0; i < ages.size(); i++)
            {
                qb["PfPR by Age Bin"][n][i] = Number(PfPRs_by_agebin.at(n).at(i));
                qb["Pf Gametocyte Prevalence by Age Bin"][n][i] = Number(PfgamPRs_by_agebin.at(n).at(i));
                qb["Mean Log Parasite Density"][n][i] = Number(mean_log_parasite_density_by_agebin.at(n).at(i));
                qb["RDT PfPR by Age Bin"][n][i] = Number(RDT_PfPRs_by_agebin.at(n).at(i));
                qb["Annual Clinical Incidence by Age Bin"][n][i] = Number(annual_clinical_incidences_by_agebin.at(n).at(i));
                qb["Annual Severe Incidence by Age Bin"][n][i] = Number(annual_severe_incidences_by_agebin.at(n).at(i));
                qb["Average Population by Age Bin"][n][i] = Number(average_population_by_agebin.at(n).at(i));
                qb["Annual Severe Incidence by Anemia by Age Bin"][n][i] = Number(annual_severe_incidences_by_anemia_by_agebin.at(n).at(i));
                qb["Annual Severe Incidence by Parasites by Age Bin"][n][i] = Number(annual_severe_incidences_by_parasites_by_agebin.at(n).at(i));
                qb["Annual Severe Incidence by Fever by Age Bin"][n][i] = Number(annual_severe_incidences_by_fever_by_agebin.at(n).at(i));
                qb["Annual Severe Anemia"][n][i] = Number(annual_severe_anemia_by_agebin.at(n).at(i));
                qb["Annual Moderate Anemia"][n][i] = Number(annual_moderate_anemia_by_agebin.at(n).at(i));
                qb["Annual Mild Anemia"][n][i] = Number(annual_mild_anemia_by_agebin.at(n).at(i));

                for (int j = 0; j < PfPRbins.size(); j++)
                {
                    qb["PfPR by Parasitemia and Age Bin"][n][j][i] = Number(binned_PfPRs_by_agebin.at(n).at(j).at(i));
                    qb["PfPR by Gametocytemia and Age Bin"][n][j][i] = Number(binned_PfgamPRs_by_agebin.at(n).at(j).at(i));
                }
            }
        }

        // write to an internal buffer first... if we write directly to the network share, performance is slow
        // (presumably because it's doing a bunch of really small writes of all the JSON elements instead of one
        // big write)
        ostringstream oss;
        Writer::Write(elementRoot, oss);

        ofstream report_json;
        report_json.open(FileSystem::Concat( EnvPtr->OutputPath, output_file_name.str() ).c_str());

        if (report_json.is_open())
        {
            report_json << oss.str();
            report_json.close();
        }
        else
        {
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, output_file_name.str().c_str() );
        }
    }
}