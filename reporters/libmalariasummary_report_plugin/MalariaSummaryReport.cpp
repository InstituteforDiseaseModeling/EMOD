/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "Serializer.h"

#include "DllInterfaceHelper.h"
#include "DllDefs.h"
#include "ProgVersion.h"

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "MalariaSummaryReport" ) // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = {"MALARIA_SIM", "DENGUE_SIM", nullptr}; // <<< Types of simulation the report is to be used with

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
// --- ReportIntervalData Methods
// ----------------------------------------

    ReportIntervalData::ReportIntervalData()
    : IIntervalData()
    , sum_EIR(0.0)
    , sum_population_2to10(0.0)
    , sum_parasite_positive_2to10(0.0)
    , sum_population_by_agebin()
    , sum_parasite_positive_by_agebin()
    , sum_gametocyte_positive_by_agebin()
    , sum_log_parasite_density_by_agebin()
    , sum_rdt_positive_by_agebin()
    , sum_clinical_cases_by_agebin()
    , sum_severe_cases_by_agebin()
    , sum_severe_anemia_by_agebin()
    , sum_moderate_anemia_by_agebin()
    , sum_mild_anemia_by_agebin()
    , sum_severe_cases_by_anemia_by_agebin()
    , sum_severe_cases_by_parasites_by_agebin()
    , sum_severe_cases_by_fever_by_agebin()
    , sum_binned_PfPR_by_agebin()
    , sum_binned_PfgamPR_by_agebin()
    , sum_no_infected_days(0.0)
    , sum_days_under_1pct_infected(0.0)
    {
    }

    ReportIntervalData::~ReportIntervalData()
    {
    }

    void ReportIntervalData::SetVectorSize( int age_size, int PfPR_size )
    {
        sum_population_by_agebin.resize(                age_size, 0 );
        sum_log_parasite_density_by_agebin.resize(      age_size, 0 );
        sum_parasite_positive_by_agebin.resize(         age_size, 0 );
        sum_gametocyte_positive_by_agebin.resize(       age_size, 0 );
        sum_rdt_positive_by_agebin.resize(              age_size, 0 );
        sum_clinical_cases_by_agebin.resize(            age_size, 0 );
        sum_severe_cases_by_agebin.resize(              age_size, 0 );
        sum_severe_anemia_by_agebin.resize(             age_size, 0 );
        sum_moderate_anemia_by_agebin.resize(           age_size, 0 );
        sum_mild_anemia_by_agebin.resize(               age_size, 0 );
        sum_severe_cases_by_anemia_by_agebin.resize(    age_size, 0 );
        sum_severe_cases_by_parasites_by_agebin.resize( age_size, 0 );
        sum_severe_cases_by_fever_by_agebin.resize(     age_size, 0 );

        for( int j = 0 ; j < PfPR_size ; ++j )
        {
            sum_binned_PfPR_by_agebin.push_back(    agebinned_t(age_size) );
            sum_binned_PfgamPR_by_agebin.push_back( agebinned_t(age_size) );
        }
    }

    void ReportIntervalData::Clear()
    {
        sum_EIR                     = 0;
        sum_population_2to10        = 0;
        sum_parasite_positive_2to10 = 0;

        std::fill( sum_population_by_agebin.begin(),                sum_population_by_agebin.end(),                0 );
        std::fill( sum_parasite_positive_by_agebin.begin(),         sum_parasite_positive_by_agebin.end(),         0 );
        std::fill( sum_gametocyte_positive_by_agebin.begin(),       sum_gametocyte_positive_by_agebin.end(),       0 );
        std::fill( sum_log_parasite_density_by_agebin.begin(),      sum_log_parasite_density_by_agebin.end(),      0 );
        std::fill( sum_rdt_positive_by_agebin.begin(),              sum_rdt_positive_by_agebin.end(),              0 );
        std::fill( sum_clinical_cases_by_agebin.begin(),            sum_clinical_cases_by_agebin.end(),            0 );
        std::fill( sum_severe_cases_by_agebin.begin(),              sum_severe_cases_by_agebin.end(),              0 );
        std::fill( sum_severe_anemia_by_agebin.begin(),             sum_severe_anemia_by_agebin.end(),             0 );
        std::fill( sum_moderate_anemia_by_agebin.begin(),           sum_moderate_anemia_by_agebin.end(),           0 );
        std::fill( sum_mild_anemia_by_agebin.begin(),               sum_mild_anemia_by_agebin.end(),               0 );
        std::fill( sum_severe_cases_by_anemia_by_agebin.begin(),    sum_severe_cases_by_anemia_by_agebin.end(),    0 );
        std::fill( sum_severe_cases_by_parasites_by_agebin.begin(), sum_severe_cases_by_parasites_by_agebin.end(), 0 );
        std::fill( sum_severe_cases_by_fever_by_agebin.begin(),     sum_severe_cases_by_fever_by_agebin.end(),     0 );

        for(int j = 0; j < sum_binned_PfPR_by_agebin.size(); j++)
        {
            std::fill( sum_binned_PfPR_by_agebin.at(j).begin(),    sum_binned_PfPR_by_agebin.at(j).end(),    0 );
            std::fill( sum_binned_PfgamPR_by_agebin.at(j).begin(), sum_binned_PfgamPR_by_agebin.at(j).end(), 0 );
        }
        // N.B. don't reset running count of no-infection streak here
        // sum_no_infected_days reset if GetInfected() > 0 in Update()
        sum_days_under_1pct_infected = 0;
    }

    void ReportIntervalData::Update( const IIntervalData& rOtherIntervalData )
    {
        const ReportIntervalData& rOther = static_cast<const ReportIntervalData&>(rOtherIntervalData);

        this->sum_EIR                      += rOther.sum_EIR;
        this->sum_population_2to10         += rOther.sum_population_2to10;
        this->sum_parasite_positive_2to10  += rOther.sum_parasite_positive_2to10;
        this->sum_no_infected_days         += rOther.sum_no_infected_days;
        this->sum_days_under_1pct_infected += rOther.sum_days_under_1pct_infected;

        ReportUtilities::AddVector( this->sum_population_by_agebin               , rOther.sum_population_by_agebin                );
        ReportUtilities::AddVector( this->sum_parasite_positive_by_agebin        , rOther.sum_parasite_positive_by_agebin         );
        ReportUtilities::AddVector( this->sum_gametocyte_positive_by_agebin      , rOther.sum_gametocyte_positive_by_agebin       );
        ReportUtilities::AddVector( this->sum_log_parasite_density_by_agebin     , rOther.sum_log_parasite_density_by_agebin      );
        ReportUtilities::AddVector( this->sum_rdt_positive_by_agebin             , rOther.sum_rdt_positive_by_agebin              );
        ReportUtilities::AddVector( this->sum_clinical_cases_by_agebin           , rOther.sum_clinical_cases_by_agebin            );
        ReportUtilities::AddVector( this->sum_severe_cases_by_agebin             , rOther.sum_severe_cases_by_agebin              );
        ReportUtilities::AddVector( this->sum_severe_anemia_by_agebin            , rOther.sum_severe_anemia_by_agebin             );
        ReportUtilities::AddVector( this->sum_moderate_anemia_by_agebin          , rOther.sum_moderate_anemia_by_agebin           );
        ReportUtilities::AddVector( this->sum_mild_anemia_by_agebin              , rOther.sum_mild_anemia_by_agebin               );
        ReportUtilities::AddVector( this->sum_severe_cases_by_anemia_by_agebin   , rOther.sum_severe_cases_by_anemia_by_agebin    );
        ReportUtilities::AddVector( this->sum_severe_cases_by_parasites_by_agebin, rOther.sum_severe_cases_by_parasites_by_agebin );
        ReportUtilities::AddVector( this->sum_severe_cases_by_fever_by_agebin    , rOther.sum_severe_cases_by_fever_by_agebin     );

        release_assert( this->sum_binned_PfPR_by_agebin.size() == rOther.sum_binned_PfPR_by_agebin.size() );
        for( int j = 0 ; j < this->sum_binned_PfPR_by_agebin.size() ; ++j )
        {
            ReportUtilities::AddVector( this->sum_binned_PfPR_by_agebin[j], rOther.sum_binned_PfPR_by_agebin.at(j) );
        }

        release_assert( this->sum_binned_PfgamPR_by_agebin.size() == rOther.sum_binned_PfgamPR_by_agebin.size() );
        for( int j = 0 ; j < this->sum_binned_PfgamPR_by_agebin.size() ; ++j )
        {
            ReportUtilities::AddVector( this->sum_binned_PfgamPR_by_agebin[j], rOther.sum_binned_PfgamPR_by_agebin.at(j) );
        }
    }

    void ReportIntervalData::Serialize( IJsonObjectAdapter& root, JSerializer& js )
    {
        root.Insert( "EIR"                      , sum_EIR                      );
        root.Insert( "population_2to10"         , sum_population_2to10         );
        root.Insert( "parasite_positive_2to10"  , sum_parasite_positive_2to10  );
        root.Insert( "no_infected_days"         , sum_no_infected_days         );
        root.Insert( "days_under_1pct_infected" , sum_days_under_1pct_infected );

        ReportUtilities::SerializeVector( root, js, "population"               , sum_population_by_agebin                );
        ReportUtilities::SerializeVector( root, js, "parasite_positive"        , sum_parasite_positive_by_agebin         );
        ReportUtilities::SerializeVector( root, js, "gametocyte_positive"      , sum_gametocyte_positive_by_agebin       );
        ReportUtilities::SerializeVector( root, js, "log_parasite_density"     , sum_log_parasite_density_by_agebin      );
        ReportUtilities::SerializeVector( root, js, "rdt_positive"             , sum_rdt_positive_by_agebin              );
        ReportUtilities::SerializeVector( root, js, "clinical_cases"           , sum_clinical_cases_by_agebin            );
        ReportUtilities::SerializeVector( root, js, "severe_cases"             , sum_severe_cases_by_agebin              );
        ReportUtilities::SerializeVector( root, js, "severe_anemia"            , sum_severe_anemia_by_agebin             );
        ReportUtilities::SerializeVector( root, js, "moderate_anemia"          , sum_moderate_anemia_by_agebin           );
        ReportUtilities::SerializeVector( root, js, "mild_anemia"              , sum_mild_anemia_by_agebin               );
        ReportUtilities::SerializeVector( root, js, "severe_cases_by_anemia"   , sum_severe_cases_by_anemia_by_agebin    );
        ReportUtilities::SerializeVector( root, js, "severe_cases_by_parasites", sum_severe_cases_by_parasites_by_agebin );
        ReportUtilities::SerializeVector( root, js, "severe_cases_by_fever"    , sum_severe_cases_by_fever_by_agebin     );

        root.Insert( "binned_PfPR" );
        root.BeginObject();
        for( int j = 0 ; j < this->sum_binned_PfPR_by_agebin.size() ; ++j )
        {
            std::stringstream ss;
            ss << "j=" << j;
            ReportUtilities::SerializeVector( root, js, ss.str().c_str(), sum_binned_PfPR_by_agebin.at(j) );
        }
        root.EndObject();

        root.Insert( "binned_PfgamPR" );
        root.BeginObject();
        for( int j = 0 ; j < this->sum_binned_PfgamPR_by_agebin.size() ; ++j )
        {
            std::stringstream ss;
            ss << "j=" << j;
            ReportUtilities::SerializeVector( root, js, ss.str().c_str(), sum_binned_PfgamPR_by_agebin.at(j) );
        }
        root.EndObject();
    }

    void ReportIntervalData::Deserialize( IJsonObjectAdapter& root )
    {
        sum_EIR                      = root.GetFloat( "EIR"                      );
        sum_population_2to10         = root.GetFloat( "population_2to10"         );
        sum_parasite_positive_2to10  = root.GetFloat( "parasite_positive_2to10"  );
        sum_no_infected_days         = root.GetFloat( "no_infected_days"         );
        sum_days_under_1pct_infected = root.GetFloat( "days_under_1pct_infected" );

        ReportUtilities::DeserializeVector( root, true, "population"               , sum_population_by_agebin                );
        ReportUtilities::DeserializeVector( root, true, "parasite_positive"        , sum_parasite_positive_by_agebin         );
        ReportUtilities::DeserializeVector( root, true, "gametocyte_positive"      , sum_gametocyte_positive_by_agebin       );
        ReportUtilities::DeserializeVector( root, true, "log_parasite_density"     , sum_log_parasite_density_by_agebin      );
        ReportUtilities::DeserializeVector( root, true, "rdt_positive"             , sum_rdt_positive_by_agebin              );
        ReportUtilities::DeserializeVector( root, true, "clinical_cases"           , sum_clinical_cases_by_agebin            );
        ReportUtilities::DeserializeVector( root, true, "severe_cases"             , sum_severe_cases_by_agebin              );
        ReportUtilities::DeserializeVector( root, true, "severe_anemia"            , sum_severe_anemia_by_agebin             );
        ReportUtilities::DeserializeVector( root, true, "moderate_anemia"          , sum_moderate_anemia_by_agebin           );
        ReportUtilities::DeserializeVector( root, true, "mild_anemia"              , sum_mild_anemia_by_agebin               );
        ReportUtilities::DeserializeVector( root, true, "severe_cases_by_anemia"   , sum_severe_cases_by_anemia_by_agebin    );
        ReportUtilities::DeserializeVector( root, true, "severe_cases_by_parasites", sum_severe_cases_by_parasites_by_agebin );
        ReportUtilities::DeserializeVector( root, true, "severe_cases_by_fever"    , sum_severe_cases_by_fever_by_agebin     );

        IJsonObjectAdapter* p_obj1 = root.GetJsonObject( "binned_PfPR" );
        for( int j = 0 ; j < sum_binned_PfPR_by_agebin.size() ; ++j )
        {
            std::stringstream ss;
            ss << "j=" << j;
            ReportUtilities::DeserializeVector( *p_obj1, true, ss.str().c_str(), sum_binned_PfPR_by_agebin[j] );
        }
        delete p_obj1;
        p_obj1 = nullptr;

        IJsonObjectAdapter* p_obj2 = root.GetJsonObject( "binned_PfgamPR" );
        for( int j = 0 ; j < sum_binned_PfgamPR_by_agebin.size() ; ++j )
        {
            std::stringstream ss;
            ss << "j=" << j;
            ReportUtilities::DeserializeVector( *p_obj2, true, ss.str().c_str(), sum_binned_PfgamPR_by_agebin[j] );
        }
        delete p_obj2;
        p_obj2 = nullptr;
    }


// ----------------------------------------
// --- MalariaSummaryReport Methods
// ----------------------------------------

    MalariaSummaryReport::MalariaSummaryReport() 
        : BaseEventReportIntervalOutput( _module, false, new ReportIntervalData(), new ReportIntervalData() ) //false => only one file
        , node_vector(nullptr)
        , ages()
        , m_pReportData(nullptr)
        , annual_EIRs()
        , PfPRs_2to10()
        , PfPRs_by_agebin()
        , PfgamPRs_by_agebin()
        , mean_log_parasite_density_by_agebin()
        , binned_PfPRs_by_agebin()
        , binned_PfgamPRs_by_agebin()
        , RDT_PfPRs_by_agebin()
        , annual_clinical_incidences_by_agebin()
        , annual_severe_incidences_by_agebin()
        , average_population_by_agebin()
        , annual_severe_incidences_by_anemia_by_agebin()
        , annual_severe_incidences_by_parasites_by_agebin()
        , annual_severe_incidences_by_fever_by_agebin()
        , annual_severe_anemia_by_agebin()
        , annual_moderate_anemia_by_agebin()
        , annual_mild_anemia_by_agebin()
        , duration_no_infection_streak()
        , fraction_under_1pct_infected()
    {
    }

    bool MalariaSummaryReport::Configure( const Configuration * inputJson )
    {
        if( inputJson->Exist("Age_Bins") )
        {
            initConfigTypeMap("Age_Bins", &ages, "Age Bins (in years) to aggregate within and report", 0, FLT_MAX, 0, true );
        }
        else
        {
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

        if( inputJson->Exist("Parasitemia_Bins") )
        {
            initConfigTypeMap("Parasitemia_Bins", &PfPRbins, "Parasitemia Bins to aggregate within and report.  A value <= 0 in the first bin indicates that the uninfected people should be added to this bin.", -FLT_MAX, FLT_MAX, 0, true);
        }
        else
        {
            // -------------------------------------------------------------
            // --- Values <= 0 in first bin indicate that uninfected people
            // --- be added to this bin.
            // -------------------------------------------------------------
            //PfPRbins.push_back(    -1.0f );
            PfPRbins.push_back(    50.0f );
            PfPRbins.push_back(   500.0f );
            PfPRbins.push_back(  5000.0f );
            PfPRbins.push_back( 50000.0f );
            PfPRbins.push_back(  FLT_MAX );
        }

        bool configured = BaseEventReportIntervalOutput::Configure( inputJson );

        if( configured )
        {
            if ((PfPRbins.size() >=2) && (PfPRbins[1] < 0))
            {
                std::ostringstream  msg;
                msg << "The value (" <<  PfPRbins[1] << ") in Parasitemia_Bins at index 1 is invalid. Value must be greater or equal 0.";
                throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, inputJson->GetDataLocation(),msg.str().c_str());
            }
         
            static_cast<ReportIntervalData*>(m_pIntervalData         )->SetVectorSize( ages.size(), PfPRbins.size() );
            static_cast<ReportIntervalData*>(m_pMulticoreDataExchange)->SetVectorSize( ages.size(), PfPRbins.size() );
            m_pReportData = static_cast<ReportIntervalData*>(m_pIntervalData);
        }
 
        return configured;
    }

    MalariaSummaryReport::~MalariaSummaryReport()
    {
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

        const VectorPopulationReportingList_t& vectorPopulations = node_vector->GetVectorPopulationReporting();
        for( auto vectorpopulation : vectorPopulations )
        {
            m_pReportData->sum_EIR += vectorpopulation->GetEIRByPool( VectorPoolIdEnum::BOTH_VECTOR_POOLS );
        }

        // add to streak or reset to zero days of no infections
        INodeContext* context = p_nec->GetNodeContext();
        if (context->GetInfected() == 0 )
        {
            m_pReportData->sum_no_infected_days += 1;
        }
        else
        {
            m_pReportData->sum_no_infected_days = 0;
        }

        if( (double(context->GetInfected()) / double(context->GetStatPop())) < 0.01 )
        {
            m_pReportData->sum_days_under_1pct_infected += 1;
        }

        BaseEventReportIntervalOutput::EndTimestep( currentTime, dt );
    }

    bool MalariaSummaryReport::notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
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
            m_pReportData->sum_population_2to10 += mc_weight;
        }
        m_pReportData->sum_population_by_agebin.at(agebin) += mc_weight;

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
                    m_pReportData->sum_parasite_positive_2to10 += mc_weight;
                }
                m_pReportData->sum_parasite_positive_by_agebin.at(agebin) += mc_weight;

                float log10_parasite_count = log10(parasite_count);
                if( !( _isnanf ( log10_parasite_count ) ) && _finitef( log10_parasite_count ) )
                {
                    m_pReportData->sum_log_parasite_density_by_agebin.at(agebin) += log10_parasite_count;
                }
            }

            int PfPRbin = GetPfPRBin(susceptibility_malaria->get_parasite_density());
            m_pReportData->sum_binned_PfPR_by_agebin.at(PfPRbin).at(agebin) += mc_weight;

            float gametocyte_count = individual_malaria->GetGametocyteDensity();
            int PfgamPRbin = GetPfPRBin(gametocyte_count);
            m_pReportData->sum_binned_PfgamPR_by_agebin.at(PfgamPRbin).at(agebin) += mc_weight;

            gametocyte_count = individual_malaria->CheckGametocyteCountWithTest(MALARIA_TEST_BLOOD_SMEAR);
            if (gametocyte_count > 0.02)
            {
                m_pReportData->sum_gametocyte_positive_by_agebin.at(agebin) += mc_weight;
            }

            
            float hemoglobin = susceptibility_malaria->GetHemoglobin();
            if ( hemoglobin < 5 )  { m_pReportData->sum_severe_anemia_by_agebin.at(agebin)   += mc_weight; }
            if ( hemoglobin < 8 )  { m_pReportData->sum_moderate_anemia_by_agebin.at(agebin) += mc_weight; }
            if ( hemoglobin < 11 ) { m_pReportData->sum_mild_anemia_by_agebin.at(agebin)     += mc_weight; }
            
            if (individual_malaria->CheckForParasitesWithTest(MALARIA_TEST_NEW_DIAGNOSTIC))
            {
                m_pReportData->sum_rdt_positive_by_agebin.at(agebin) += mc_weight;
            }

            if (individual_malaria->HasClinicalSymptom(ClinicalSymptomsEnum::CLINICAL_DISEASE))
            {
                m_pReportData->sum_clinical_cases_by_agebin.at(agebin) += mc_weight;
            }
            if (individual_malaria->HasClinicalSymptom(ClinicalSymptomsEnum::SEVERE_DISEASE))
            {
                m_pReportData->sum_severe_cases_by_agebin.at(agebin) += mc_weight;
                switch( susceptibility_malaria->CheckSevereCaseType() )
                {
                    case Kernel::SevereCaseTypesEnum::ANEMIA:
                        m_pReportData->sum_severe_cases_by_anemia_by_agebin.at(agebin) += mc_weight;
                    break;

                    case Kernel::SevereCaseTypesEnum::PARASITES:
                        m_pReportData->sum_severe_cases_by_parasites_by_agebin.at(agebin) += mc_weight;
                    break;

                    case Kernel::SevereCaseTypesEnum::FEVER:
                        m_pReportData->sum_severe_cases_by_fever_by_agebin.at(agebin) += mc_weight;
                    break;
                }
            }
        } //end if infected
        else if( PfPRbins[0] <= 0.0f )
        {
            // ----------------------------------------------------------------------------------------------
            // --- If specified by the user, add the people who are not infected to the first bin.
            // --- NOTE: There can be people who are infected but their "parasite density" (i.e. parasitemia)
            // --- is zero.  Hence, if the first bin is zero, then it will contain both those that are
            // --- uninfected as well as those that are infected but have zero parasitemia.
            // ----------------------------------------------------------------------------------------------
            m_pReportData->sum_binned_PfPR_by_agebin.at( 0 ).at( agebin ) += mc_weight;
            m_pReportData->sum_binned_PfgamPR_by_agebin.at( 0 ).at( agebin ) += mc_weight;
        }

        return true;
    }

    int MalariaSummaryReport::GetPfPRBin(float parasite_count)
    {
        vector<float>::const_iterator it;
        it = std::lower_bound(PfPRbins.begin(), PfPRbins.end(), parasite_count);
        int PfPRbin_idx = it - PfPRbins.begin();
        if( PfPRbin_idx >= PfPRbins.size() )
        {
            LOG_WARN_F( "Invalid PfPRbin_idx=%d  parasite_count=%f  Num bins=%d.  Make last bin value larger.  Putting in last bin.\n", PfPRbin_idx, parasite_count, PfPRbins.size() );
            PfPRbin_idx = PfPRbins.size() - 1;
        }
        return PfPRbin_idx;
    }

    void MalariaSummaryReport::AccumulateOutput()
    {
        LOG_INFO_F("Aggregating output on reporting interval of %d days\n", (int)m_reporting_interval);
        time_of_report.push_back( m_current_time );
        annual_EIRs.push_back( 365.0 * m_pReportData->sum_EIR / m_reporting_interval );

        if( m_pReportData->sum_population_2to10 > 0 )
        {
            PfPRs_2to10.push_back( m_pReportData->sum_parasite_positive_2to10 / m_pReportData->sum_population_2to10 );
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
            average_population.push_back(m_pReportData->sum_population_by_agebin.at(i) * 1.0/m_reporting_interval);
            if( m_pReportData->sum_population_by_agebin.at(i) > 0)
            {
                float sum_pop_by_agebin = m_pReportData->sum_population_by_agebin.at(i);
                float days_per_year_per_pop = 365.0 / sum_pop_by_agebin;

                pfprs.push_back(     m_pReportData->sum_parasite_positive_by_agebin.at(i)   / sum_pop_by_agebin );
                pfgamprs.push_back(  m_pReportData->sum_gametocyte_positive_by_agebin.at(i) / sum_pop_by_agebin );
                rdt_pfprs.push_back( m_pReportData->sum_rdt_positive_by_agebin.at(i)        / sum_pop_by_agebin );

                annual_clinical_incidences.push_back(            days_per_year_per_pop * m_pReportData->sum_clinical_cases_by_agebin.at(i)            );
                annual_severe_incidences.push_back(              days_per_year_per_pop * m_pReportData->sum_severe_cases_by_agebin.at(i)              );
                annual_severe_incidences_by_anemia.push_back(    days_per_year_per_pop * m_pReportData->sum_severe_cases_by_anemia_by_agebin.at(i)    );
                annual_severe_incidences_by_parasites.push_back( days_per_year_per_pop * m_pReportData->sum_severe_cases_by_parasites_by_agebin.at(i) );
                annual_severe_incidences_by_fever.push_back(     days_per_year_per_pop * m_pReportData->sum_severe_cases_by_fever_by_agebin.at(i)     );
                annual_severe_anemia.push_back(                  days_per_year_per_pop * m_pReportData->sum_severe_anemia_by_agebin.at(i)             );
                annual_moderate_anemia.push_back(                days_per_year_per_pop * m_pReportData->sum_moderate_anemia_by_agebin.at(i)           );
                annual_mild_anemia.push_back(                    days_per_year_per_pop * m_pReportData->sum_mild_anemia_by_agebin.at(i)               );

                if( m_pReportData->sum_parasite_positive_by_agebin.at(i) > 0 )
                {
                    log_parasite_density.push_back( m_pReportData->sum_log_parasite_density_by_agebin.at(i) / m_pReportData->sum_parasite_positive_by_agebin.at(i) );
                }
                else
                {
                    log_parasite_density.push_back(0.0);
                }

                for(int j = 0; j<PfPRbins.size(); j++)
                {
                    pfprs_binned.at(j).at(i)    = m_pReportData->sum_binned_PfPR_by_agebin.at(j).at(i)    / sum_pop_by_agebin;
                    pfgamprs_binned.at(j).at(i) = m_pReportData->sum_binned_PfgamPR_by_agebin.at(j).at(i) / sum_pop_by_agebin;
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
            }
        }

        PfPRs_by_agebin.push_back(     pfprs     );
        PfgamPRs_by_agebin.push_back(  pfprs     );
        RDT_PfPRs_by_agebin.push_back( rdt_pfprs );

        mean_log_parasite_density_by_agebin.push_back( log_parasite_density );
        average_population_by_agebin.push_back(        average_population   );

        annual_clinical_incidences_by_agebin.push_back(            annual_clinical_incidences            );
        annual_severe_incidences_by_agebin.push_back(              annual_severe_incidences              );
        annual_severe_incidences_by_anemia_by_agebin.push_back(    annual_severe_incidences_by_anemia    );
        annual_severe_incidences_by_parasites_by_agebin.push_back( annual_severe_incidences_by_parasites );
        annual_severe_incidences_by_fever_by_agebin.push_back(     annual_severe_incidences_by_fever     );
        annual_severe_anemia_by_agebin.push_back(                  annual_severe_anemia                  );
        annual_moderate_anemia_by_agebin.push_back(                annual_moderate_anemia                );
        annual_mild_anemia_by_agebin.push_back(                    annual_mild_anemia                    );

        binned_PfPRs_by_agebin.push_back(    pfprs_binned    );
        binned_PfgamPRs_by_agebin.push_back( pfgamprs_binned );

        duration_no_infection_streak.push_back( m_pReportData->sum_no_infected_days );
        fraction_under_1pct_infected.push_back( m_pReportData->sum_days_under_1pct_infected / m_reporting_interval );
    }

    void MalariaSummaryReport::SerializeOutput( float currentTime, IJsonObjectAdapter& output, JSerializer& js )
    {
        output.Insert( "Metadata" );
        output.BeginObject();
        output.Insert("Start_Day", GetStartDay() );
        output.Insert("Reporting_Interval", m_reporting_interval );
        ReportUtilities::SerializeVector( output, js, "Age Bins"           , ages     );
        ReportUtilities::SerializeVector( output, js, "Parasitemia Bins"   , PfPRbins );
        ReportUtilities::SerializeVector( output, js, "Gametocytemia Bins" , PfPRbins ); // not sure why same data
        output.EndObject();

        output.Insert( "DataByTime" );
        output.BeginObject();
        ReportUtilities::SerializeVector( output, js, "Time Of Report"                    , time_of_report               );
        ReportUtilities::SerializeVector( output, js, "Annual EIR"                        , annual_EIRs                  );
        ReportUtilities::SerializeVector( output, js, "PfPR_2to10"                        , PfPRs_2to10                  );
        ReportUtilities::SerializeVector( output, js, "No Infection Streak"               , duration_no_infection_streak );
        ReportUtilities::SerializeVector( output, js, "Fraction Days Under 1pct Infected" , fraction_under_1pct_infected );
        output.EndObject();

        output.Insert( "DataByTimeAndAgeBins" );
        output.BeginObject();
        ReportUtilities::SerializeVector2D( output, js, "PfPR by Age Bin"                                 , PfPRs_by_agebin                                 );
        ReportUtilities::SerializeVector2D( output, js, "Pf Gametocyte Prevalence by Age Bin"             , PfgamPRs_by_agebin                              );
        ReportUtilities::SerializeVector2D( output, js, "Mean Log Parasite Density"                       , mean_log_parasite_density_by_agebin             );
        ReportUtilities::SerializeVector2D( output, js, "RDT PfPR by Age Bin"                             , RDT_PfPRs_by_agebin                             );
        ReportUtilities::SerializeVector2D( output, js, "Annual Clinical Incidence by Age Bin"            , annual_clinical_incidences_by_agebin            );
        ReportUtilities::SerializeVector2D( output, js, "Annual Severe Incidence by Age Bin"              , annual_severe_incidences_by_agebin              );
        ReportUtilities::SerializeVector2D( output, js, "Average Population by Age Bin"                   , average_population_by_agebin                    );
        ReportUtilities::SerializeVector2D( output, js, "Annual Severe Incidence by Anemia by Age Bin"    , annual_severe_incidences_by_anemia_by_agebin    );
        ReportUtilities::SerializeVector2D( output, js, "Annual Severe Incidence by Parasites by Age Bin" , annual_severe_incidences_by_parasites_by_agebin );
        ReportUtilities::SerializeVector2D( output, js, "Annual Severe Incidence by Fever by Age Bin"     , annual_severe_incidences_by_fever_by_agebin     );
        ReportUtilities::SerializeVector2D( output, js, "Annual Severe Anemia"                            , annual_severe_anemia_by_agebin                  );
        ReportUtilities::SerializeVector2D( output, js, "Annual Moderate Anemia"                          , annual_moderate_anemia_by_agebin                );
        ReportUtilities::SerializeVector2D( output, js, "Annual Mild Anemia"                              , annual_mild_anemia_by_agebin                    );
        output.EndObject();

        output.Insert( "DataByTimeAndPfPRBinsAndAgeBins" );
        output.BeginObject();
        ReportUtilities::SerializeVector3D( output, js, "PfPR by Parasitemia and Age Bin"   , binned_PfPRs_by_agebin    );
        ReportUtilities::SerializeVector3D( output, js, "PfPR by Gametocytemia and Age Bin" , binned_PfgamPRs_by_agebin );
        output.EndObject();
    }
}
