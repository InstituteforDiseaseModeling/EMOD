/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "StdAfx.h"
#include "MalariaImmunityReport.h"

#include <algorithm>

#include "DllInterfaceHelper.h"
#include "FileSystem.h"
#include "Environment.h"
#include "Exceptions.h"
#include "IndividualMalaria.h"
#include "SusceptibilityMalaria.h"
#include "ReportUtilities.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Module name for logging, CustomReport.json, and DLL GetType()
static const char * _module = "MalariaImmunityReport"; // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", nullptr}; // <<< Types of simulation the report is to be used with

Kernel::report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new MalariaImmunityReport()); // <<< Report to create
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
// --- MalariaImmunityReport Methods
// ----------------------------------------

    MalariaImmunityReport::MalariaImmunityReport() 
        : BaseEventReportIntervalOutput( _module )
        , m_has_data(false)
        , ages()
        , sum_population_by_agebin()
        , sum_MSP_by_agebin()
        , sum_nonspec_by_agebin()
        , sum_pfemp1_by_agebin()
        , sumsqr_MSP_by_agebin()
        , sumsqr_nonspec_by_agebin()
        , sumsqr_pfemp1_by_agebin()
        , MSP_mean_by_agebin()
        , MSP_std_by_agebin()
        , nonspec_mean_by_agebin()
        , nonspec_std_by_agebin()
        , PfEMP1_mean_by_agebin()
        , PfEMP1_std_by_agebin()
    {
    }

    bool MalariaImmunityReport::Configure( const Configuration * inputJson )
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

        sum_population_by_agebin.resize( ages.size(), 0);
        sum_MSP_by_agebin.resize(        ages.size(), 0);
        sum_nonspec_by_agebin.resize(    ages.size(), 0);
        sum_pfemp1_by_agebin.resize(     ages.size(), 0);
        sumsqr_MSP_by_agebin.resize(     ages.size(), 0);
        sumsqr_nonspec_by_agebin.resize( ages.size(), 0);
        sumsqr_pfemp1_by_agebin.resize(  ages.size(), 0);

        return configured;
    }

    MalariaImmunityReport::~MalariaImmunityReport()
    {
        if( m_has_data )
        {
            LOG_WARN("Summary report not written yet, but the simulation is over.  Writing now...\n");
            AccumulateOutput();
            WriteOutput(-999.0);
        }
        ClearOutputData();
    }

    void MalariaImmunityReport::EndTimestep( float currentTime, float dt )
    {
        if( HaveUnregisteredAllEvents() )
        {
            // --------------------------------------------------------------------------------
            // --- If we have either not registered or unregistered listening for events, then
            // --- we don't want to consider outputing data.
            // --------------------------------------------------------------------------------
            return;
        }
        else if( HaveRegisteredAllEvents() )
        {
            m_interval_timer++;

            LOG_DEBUG_F("m_interval_timer=%d, m_reporting_interval=%d\n",m_interval_timer,m_reporting_interval);
            if ( m_interval_timer >= m_reporting_interval )
            {
                m_report_count++;
                AccumulateOutput();

                LOG_DEBUG_F("Resetting %s reporting interval timer...\n", GetReportName().c_str());
                m_interval_timer = 0 ;

                if ( m_report_count >= m_max_number_reports )
                {
                    WriteOutput( currentTime );
                    UnregisterAllNodes();
                    ClearOutputData();
                    m_has_data = false ;
                }
            }
        }
    }

    bool MalariaImmunityReport::notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange )
    {
        LOG_DEBUG_F( "MalariaSummaryReport notified of event by %d-year old individual.\n", (int) (context->GetAge() / DAYSPERYEAR) );

        // individual context for suid
        IIndividualHumanContext * iindividual = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanContext), (void**)&iindividual) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanContext", "IIndividualHumanEventContext");
        }
        m_has_data = true ;

        int    id        = iindividual->GetSuid().data;
        double mc_weight = context->GetMonteCarloWeight();
        double age       = context->GetAge();
        int    agebin    = ReportUtilities::GetAgeBin( (float)age, ages );

        sum_population_by_agebin.at(agebin) += mc_weight;

        // get malaria contexts
        IMalariaHumanContext * individual_malaria = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IMalariaHumanContext), (void**)&individual_malaria) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IMalariaHumanContext", "IIndividualHumanEventContext");
        }
        IMalariaSusceptibility* susceptibility_malaria = individual_malaria->GetMalariaSusceptibilityContext();

        // push back fraction of immune variants to which individual has been exposed
        float msp = susceptibility_malaria->get_fraction_of_variants_with_antibodies(Kernel::MalariaAntibodyType::MSP1);
        sum_MSP_by_agebin.at(agebin) += mc_weight * msp;
        sumsqr_MSP_by_agebin.at(agebin) += mc_weight * msp * msp;
        float nonspec = susceptibility_malaria->get_fraction_of_variants_with_antibodies(Kernel::MalariaAntibodyType::PfEMP1_minor);
        sum_nonspec_by_agebin.at(agebin) += mc_weight * nonspec;
        sumsqr_nonspec_by_agebin.at(agebin) += mc_weight * nonspec * nonspec;
        float pfemp1 = susceptibility_malaria->get_fraction_of_variants_with_antibodies(Kernel::MalariaAntibodyType::PfEMP1_major);
        sum_pfemp1_by_agebin.at(agebin) += mc_weight * pfemp1;
        sumsqr_pfemp1_by_agebin.at(agebin) += mc_weight * pfemp1 * pfemp1;

        return true;
    }

    void MalariaImmunityReport::AccumulateOutput()
    {
        LOG_INFO_F("Aggregating output on reporting interval of %d days\n", (int)m_reporting_interval);

        agebinned_t msp_means;
        agebinned_t nonspec_means;
        agebinned_t pfemp1_means;

        agebinned_t msp_stds;
        agebinned_t nonspec_stds;
        agebinned_t pfemp1_stds;

        for(int i = 0; i<ages.size(); i++)
        {
            if (sum_population_by_agebin.at(i) > 0)
            {
                double mean_msp     = sum_MSP_by_agebin.at(i)     / sum_population_by_agebin.at(i) ;
                double mean_nonspec = sum_nonspec_by_agebin.at(i) / sum_population_by_agebin.at(i) ;
                double mean_pfemp1  = sum_pfemp1_by_agebin.at(i)  / sum_population_by_agebin.at(i) ;

                msp_means.push_back(     mean_msp     );
                nonspec_means.push_back( mean_nonspec );
                pfemp1_means.push_back(  mean_pfemp1  );

                msp_stds.push_back(     sqrt( (sumsqr_MSP_by_agebin.at(i)     / sum_population_by_agebin.at(i)) - (mean_msp     * mean_msp    ) ) );
                nonspec_stds.push_back( sqrt( (sumsqr_nonspec_by_agebin.at(i) / sum_population_by_agebin.at(i)) - (mean_nonspec * mean_nonspec) ) );
                pfemp1_stds.push_back(  sqrt( (sumsqr_pfemp1_by_agebin.at(i)  / sum_population_by_agebin.at(i)) - (mean_pfemp1  * mean_pfemp1 ) ) );
            }
            else
            {
                msp_means.push_back(0.0);
                nonspec_means.push_back(0.0);
                pfemp1_means.push_back(0.0);

                msp_stds.push_back(0.0);
                nonspec_stds.push_back(0.0);
                pfemp1_stds.push_back(0.0);
            }
        }

        MSP_mean_by_agebin.push_back(     msp_means     );
        nonspec_mean_by_agebin.push_back( nonspec_means );
        PfEMP1_mean_by_agebin.push_back(  pfemp1_means  );
        MSP_std_by_agebin.push_back(      msp_stds      );
        nonspec_std_by_agebin.push_back(  nonspec_stds  );
        PfEMP1_std_by_agebin.push_back(   pfemp1_stds   );

        std::fill( sum_population_by_agebin.begin(), sum_population_by_agebin.end(), 0);
        std::fill( sum_MSP_by_agebin.begin(),        sum_MSP_by_agebin.end(),        0);
        std::fill( sumsqr_MSP_by_agebin.begin(),     sumsqr_MSP_by_agebin.end(),     0);
        std::fill( sum_nonspec_by_agebin.begin(),    sum_nonspec_by_agebin.end(),    0);
        std::fill( sumsqr_nonspec_by_agebin.begin(), sumsqr_nonspec_by_agebin.end(), 0);
        std::fill( sum_pfemp1_by_agebin.begin(),     sum_pfemp1_by_agebin.end(),     0);
        std::fill( sumsqr_pfemp1_by_agebin.begin(),  sumsqr_pfemp1_by_agebin.end(),  0);

    }

    void MalariaImmunityReport::WriteOutput( float currentTime )
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

        for (int n = 0; n < MSP_mean_by_agebin.size(); n++)
        {
            for (int i = 0; i < ages.size(); i++)
            {
                qb[ "MSP Mean by Age Bin"            ][n][i] = Number( MSP_mean_by_agebin.at(n).at(i));
                qb[ "Non-Specific Mean by Age Bin"   ][n][i] = Number( nonspec_mean_by_agebin.at(n).at(i));
                qb[ "PfEMP1 Mean by Age Bin"         ][n][i] = Number( PfEMP1_mean_by_agebin.at(n).at(i));
                qb[ "MSP StdDev by Age Bin"          ][n][i] = Number( MSP_std_by_agebin.at(n).at(i));
                qb[ "Non-Specific StdDev by Age Bin" ][n][i] = Number( nonspec_std_by_agebin.at(n).at(i));
                qb[ "PfEMP1 StdDev by Age Bin"       ][n][i] = Number( PfEMP1_std_by_agebin.at(n).at(i));
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