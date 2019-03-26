/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Report_TBHIV_ByAge.h"
#include "DllInterfaceHelper.h"

#include "TBContexts.h"
#include "IndividualCoInfection.h"
#include "TBInterventionsContainer.h"
#include "MasterInterventionsContainer.h"
#include "Drugs.h"
#include "AntiTBDrug.h"
#include "InfectionTB.h"

#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"
#include "SusceptibilityHIV.h"
#include "SusceptibilityTB.h"
#include "INodeContext.h"
#include "FactorySupport.h"
#include "IdmDatetime.h"

// TODO: 
// --> Start_Year
// --> Every 6 months
// --> Strings for CD4 stage and care stage
// --> Functions computing cd4_stage and care_stage
// --> Function for counter reset

// BASE_YEAR is temporary until Year() is fixed!
#define BASE_YEAR (0)
#define FIFTEEN_YEARS (15.0f * DAYSPERYEAR)
#define SIX_MONTHS (0.5f * DAYSPERYEAR)
#define MAX_AGE_YRS 200.0f
#define REPORT_PERIOD 360

// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!! CREATING NEW REPORTS
// !!! If you are creating a new report by copying this one, you will need to modify 
// !!! the values below indicated by "<<<"

// Name for logging, CustomReport.json, and DLL GetType()
SETUP_LOGGING( "Report_TBHIV_ByAge" ) // <<< Name of this file

namespace Kernel
{
// You can put 0 or more valid Sim types into _sim_types but has to end with nullptr.
// "*" can be used if it applies to all simulation types.
static const char * _sim_types[] = { "TBHIV_SIM", nullptr };// <<< Types of simulation the report is to be used with

report_instantiator_function_t rif = []()
{
    return (Kernel::IReport*)(new Report_TBHIV_ByAge()); // <<< Report to create
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
// --- Report_HIV_WHO2015 Methods
// ----------------------------------------

    Report_TBHIV_ByAge::Report_TBHIV_ByAge()
        : BaseTextReportEvents( "Report_TBHIV_ByAge.csv" )
        , report_tbhiv_half_period( REPORT_PERIOD/2.0 )
        , next_report_time(report_tbhiv_half_period)
        , doReport( false )
        , startYear(0.0)
        , stopYear(FLT_MAX)
        , is_collecting_data(false)
        , min_age_yrs(0.0f)
        , max_age_yrs(MAX_AGE_YRS)
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();

        // Add events to event trigger list

        // Call a reset function
        ZERO_ARRAY(Population);
        ZERO_ARRAY(DiseaseDeaths);
        ZERO_ARRAY(NonDiseaseDeaths);
        ZERO_ARRAY(OnART);
        ZERO_ARRAY(New_Activations);                                                 //                       --> Infections
        ZERO_ARRAY(Active_Prevalence);
        ZERO_ARRAY(Active_Sx_Prevalence);
        ZERO_ARRAY(Active_PreSymptomatic);
        ZERO_ARRAY(Active_Smear_Positive);
        ZERO_ARRAY(Latent);
        ZERO_ARRAY(HIVstatus);
        ZERO_ARRAY(HIVDeaths);
        ZERO_ARRAY(TBStartTreatment);
        Births = 0; 
        ZERO_ARRAY(TBFailedTreatment);
        ZERO_ARRAY(PotentialNotifications);
        ZERO_ARRAY(Retreatments);
        ZERO_ARRAY(PrevalentMDR);
        ZERO_ARRAY(IncidentMDR);
        ZERO_ARRAY(NewInfections);
        ZERO_ARRAY(HIVPosNewActivations);
        ZERO_ARRAY(HIVPosTBDeaths);
        ZERO_ARRAY(HIVPosNotifications);
        ZERO_ARRAY(TBTests);
        
        ZERO_ARRAY(DynamicEvents);
        ZERO_ARRAY(HIVDeathsActiveTB);
        

    }

    Report_TBHIV_ByAge::~Report_TBHIV_ByAge()
    {
        
    }

    bool Report_TBHIV_ByAge::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "Start_Year", &startYear, "Year to start collecting data ", BASE_YEAR, MAX_YEAR, BASE_YEAR );
        initConfigTypeMap( "Stop_Year",  &stopYear,  "Year to stop collecting data",  BASE_YEAR, MAX_YEAR, BASE_YEAR );
        initConfigTypeMap( "Min_Age_Yrs", &min_age_yrs, "Minimum age to aollect data", 0.0f, MAX_AGE_YRS, 0.0f);
        initConfigTypeMap("Max_Age_Yrs", &max_age_yrs, "Maximum age to collect data", 0.0f, MAX_AGE_YRS, MAX_AGE_YRS);
        initConfigTypeMap("Additional_Events", &Additional_Event_Names, "Additional Events to Record");

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret ) {
            if( startYear < BASE_YEAR ) //IdmDateTime::_base_year
            {
                startYear = BASE_YEAR;  //IdmDateTime::_base_year ;
            }
            if( startYear >= stopYear )
            {
                 throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Start_Year", startYear, "Stop_Year", stopYear );
            }
            if (max_age_yrs <= min_age_yrs)
            {
                throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Max_Age_Yrs", max_age_yrs, "Min_Age_Yrs", min_age_yrs);
            }
        }

        for (vector<string>::iterator it =  Additional_Event_Names.begin(); it != Additional_Event_Names.cend(); ++it)
        {
            eventTriggerList.push_back(EventTrigger(*it));

        }

        // Manually push required events into the eventTriggerList
        eventTriggerList.push_back( EventTrigger::TBTestPositive    );
        eventTriggerList.push_back( EventTrigger::StartedART            );
        eventTriggerList.push_back( EventTrigger::StoppedART            );
        eventTriggerList.push_back( EventTrigger::TBActivation     );
        eventTriggerList.push_back( EventTrigger::DiseaseDeaths         );
        eventTriggerList.push_back( EventTrigger::NonDiseaseDeaths      );
        eventTriggerList.push_back(EventTrigger::Births);
        eventTriggerList.push_back(EventTrigger::TBStartDrugRegimen);
        eventTriggerList.push_back(EventTrigger::TBFailedDrugRegimen);
        eventTriggerList.push_back(EventTrigger::OpportunisticInfectionDeath);
        eventTriggerList.push_back(EventTrigger::ProviderOrdersTBTest);
        eventTriggerList.push_back(EventTrigger::NewInfectionEvent);

        
        return ret;
    }

    void Report_TBHIV_ByAge::Initialize( unsigned int nrmSize )
    {
        BaseTextReportEvents::Initialize( nrmSize );

        // has to be done if Initialize() since it is called after the demographics is read
       // IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( "InterventionStatus", "", false );
       // if( p_ip != nullptr )
      //  {
         //   m_InterventionStatusKey = p_ip->GetKey<IPKey>();
       // }
    }

    void Report_TBHIV_ByAge::UpdateEventRegistration( float currentTime,
                                                      float dt, 
                                                      std::vector<INodeEventContext*>& rNodeEventContextList,
                                                      ISimulationEventContext* pSimEventContext )
    {
        // not enforcing simulation to be not null in constructor so one can create schema with it null

        release_assert( !rNodeEventContextList.empty() );

        // BASE_YEAR is TEMPORARY HERE!!!
        float current_year = BASE_YEAR + rNodeEventContextList.front()->GetTime().Year();

        if( !is_collecting_data && (startYear <= current_year) && (current_year < stopYear) )
        {
            BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
            is_collecting_data = true ;

            // ------------------------------------------------------------------------
            // --- The idea here is to ensure that as we increase the startYear from the 
            // --- base_year we get the same report times as when startYear = base_year
            // --- The difference between start and base year gives us the number of days
            // --- into the simulation that we think we should be.  It ignores issues with
            // --- the size of dt not ending exactly on integer years.  We subtract the
            // --- dt/2 to deal with rounding errors.  For example, if the half period was 182.5,
            // --- start_year == _base_year, and dt = 30, then next_report_time = 167.5 and
            // --- data would be collected at 180.  However, for the next update
            // --- next_report_time would be 350 and the update would occur at 360.
            // ------------------------------------------------------------------------
            next_report_time = DAYSPERYEAR*(startYear - BASE_YEAR) + report_tbhiv_half_period -dt/2.0 ;
        }
        else if( is_collecting_data && (current_year >= stopYear) )
        {
            UnregisterAllBroadcasters();
            is_collecting_data = false ;
        }

        if( is_collecting_data )
        {
            // Figure out when to set doReport to true.  doReport is true for those
            // timesteps where we take a snapshot, i.e., as a function of the
            // half-year offset and full-year periodicity.
            doReport = false;

            if( currentTime >= next_report_time ) 
            {
                next_report_time += report_tbhiv_half_period;

                LOG_DEBUG_F( "Setting doReport to true .\n" );
                doReport = true;
            }
        }
    }

    
    std::string Report_TBHIV_ByAge::GetHeader() const
    {
        std::stringstream header ;
        header << "Year"                  << ", "
               << "NodeID"                << ", "
               << "AgeBin"                << ", "
               << "Population"            << ", "
               << "Active"                << ", "
               << "Active_Smear_Pos"      << ", "
               << "Active_Pre_Symp"       << ", "
               << "Active_Sx"             << ", "
               << "Incidence"             << ", "
               << "DiseaseDeaths"         << ", "
               << "NonDiseaseDeaths"      << ", "
               << "Latent"                << ", "
               << "HIV"                   << ", "
               << "ART"                   << ", "
               << "Births"                << ", "
               << "HIVDeaths"             << ", "
               << "TBStartTreatment"      << ", "
               << "TBFailedTreatment"     << ", "
               << "Notifications"         << ", "
               << "Prevalent_MDR"         << ", "
               << "Incident_MDR"          << ", "
               << "New_TB_Infections"     << ", "
               << "New_HIV_Pos_Activations"   << ", "
               << "New_HIV_Pos_TB_Deaths" << ", "
               << "HIV_Pos_Notifications" << ", "
               << "Retreatments"          << ", "
               << "HIVDeaths_ActiveTB"    << ", "
               << "TB_Tests" ;
              
        for( auto it = Additional_Event_Names.begin(); it != Additional_Event_Names.cend(); ++it)
        {
            header << ", "
                << *it;
        }
            
        return header.str();

    }

    Report_Age::Enum  Report_TBHIV_ByAge::ComputeAgeBin(float loc_age)
    {

        if (loc_age < DAYSPERYEAR)
        {
            return Report_Age::LESS_1;
        }
        else if (loc_age < 5.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_5;
        }
        else if (loc_age < 10.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_10;
        }
        else if (loc_age < 15.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_15;
        }
        else if (loc_age < 20.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_20;
        }
        else if (loc_age < 25.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_25;
        }
        else if (loc_age < 30.0 *DAYSPERYEAR)
        {
            return Report_Age::LESS_30;
        }
        else if (loc_age < 35.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_35;
        }
        else if (loc_age < 40 * DAYSPERYEAR)
        {
            return Report_Age::LESS_40;
        }
        else if (loc_age < 45.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_45;
        }
        else if (loc_age < 50.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_50;
        }
        else if (loc_age < 55.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_55;
        }
        else if (loc_age < 60.0 *DAYSPERYEAR)
        {
            return Report_Age::LESS_60;
        }
        else if (loc_age < 65.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_65;
        }
        else if (loc_age < 70.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_70;
        }
        else if (loc_age < 75.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_75;
        }
        else if (loc_age < 80.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_80;
        }
        else if (loc_age < 85.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_85;
        }
        else if (loc_age < 90.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_90;
        }
        else if (loc_age < 95.0 * DAYSPERYEAR)
        {
            return Report_Age::LESS_95;
        }
        else
        {
            return Report_Age::GREAT_95;
        }
    }

    void Report_TBHIV_ByAge::LogNodeData(Kernel::INodeContext* pNC)
    {
        if ((is_collecting_data == false) || (doReport == false))
        {
            return;
        }
        LOG_DEBUG_F("%s: doReport = %d\n", __FUNCTION__, doReport);

        // BASE_YEAR is TEMPORARY HERE!
        float year = BASE_YEAR + pNC->GetTime().Year();
        int nodeId = pNC->GetExternalID();

        for (int age_idx = 0; age_idx < Report_Age::Enum::COUNT; age_idx++)
        {
            GetOutputStream() << year
                << "," << nodeId
                << "," << Report_Age::pairs::lookup_key(age_idx)
                << "," << Population[age_idx]
                << "," << Active_Prevalence[age_idx]
                << "," << Active_Smear_Positive[age_idx]
                << "," << Active_PreSymptomatic[age_idx]
                << "," << Active_Sx_Prevalence[age_idx]
                << "," << New_Activations[age_idx]
                << "," << DiseaseDeaths[age_idx]
                << "," << NonDiseaseDeaths[age_idx]
                << "," << Latent[age_idx]
                << "," << HIVstatus[age_idx]
                << "," << OnART[age_idx]
                << "," << (age_idx == (int)Report_Age::GREAT_95 ? Births : 0.0f)
                << "," << HIVDeaths[age_idx]
                << "," << TBStartTreatment[age_idx]
                << "," << TBFailedTreatment[age_idx]
                << "," << PotentialNotifications[age_idx]
                << "," << PrevalentMDR[age_idx]
                << "," << IncidentMDR[age_idx]
                << "," << NewInfections[age_idx]
                << "," << HIVPosNewActivations[age_idx]
                << "," << HIVPosTBDeaths[age_idx]
                << "," << HIVPosNotifications[age_idx]
                << "," << Retreatments[age_idx]
                << "," << HIVDeathsActiveTB[age_idx]
                << "," << TBTests[age_idx];

           for (int i = 0; i < Additional_Event_Names.size(); i++)
            {
                GetOutputStream() << ","
                    << DynamicEvents[i][ age_idx];
            } 
                     
               GetOutputStream() << endl;
        } 

        // Call a reset function
        ZERO_ARRAY(Population);
        ZERO_ARRAY(Active_Prevalence);
        ZERO_ARRAY(Active_Smear_Positive);
        ZERO_ARRAY(Active_Sx_Prevalence);
        ZERO_ARRAY(Active_PreSymptomatic);
        ZERO_ARRAY(New_Activations);
        ZERO_ARRAY(DiseaseDeaths);
        ZERO_ARRAY(NonDiseaseDeaths);
        ZERO_ARRAY(Latent);
        ZERO_ARRAY(HIVstatus);
        ZERO_ARRAY(OnART);
        Births = 0;
        ZERO_ARRAY(HIVDeaths);
        ZERO_ARRAY(TBStartTreatment);
        ZERO_ARRAY(TBFailedTreatment);
        ZERO_ARRAY(PotentialNotifications);
        ZERO_ARRAY(Retreatments);
        ZERO_ARRAY(PrevalentMDR);
        ZERO_ARRAY(IncidentMDR);
        ZERO_ARRAY(NewInfections);
        ZERO_ARRAY(HIVPosNewActivations);
        ZERO_ARRAY(HIVPosTBDeaths);
        ZERO_ARRAY(HIVPosNotifications);
        ZERO_ARRAY(TBTests);
        ZERO_ARRAY(HIVDeathsActiveTB);

        ZERO_ARRAY(DynamicEvents);


      
    }

    bool Report_TBHIV_ByAge::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return is_collecting_data && doReport; 
    }

    void Report_TBHIV_ByAge::LogIndividualData(Kernel::IIndividualHuman* individual)
    {
        if (individual->GetAge() < min_age_yrs * DAYSPERYEAR || individual->GetAge() > max_age_yrs * DAYSPERYEAR)
            return;

        float mc_weight = individual->GetMonteCarloWeight();
        IIndividualHumanHIV * iptrhiv = NULL;
        if (s_OK != individual->GetEventContext()->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&iptrhiv))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "GetEventContext", "IIndividualHumanHIV", "IIndividualHumanEventContext");
        }

        IIndividualHumanTB * iptrtb = NULL;
        if (s_OK != individual->GetEventContext()->QueryInterface(GET_IID(IIndividualHumanTB), (void**)&iptrtb))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "GetEventContext", "IIndividualHumanTB", "IIndividualHumanEventContext");
        }

        auto age_group = ComputeAgeBin(  (float) individual->GetAge() ); //age in days

        // Now increment prevalence counters
        Population[age_group] += mc_weight;

        if (iptrtb->HasActiveInfection())
        {
            Active_Prevalence[age_group] += mc_weight;
            if (iptrtb->IsMDR())
            {
                PrevalentMDR[age_group] += mc_weight;
            }

            if (iptrtb->HasActivePresymptomaticInfection())
            {
                Active_PreSymptomatic[age_group] += mc_weight;
            }
            else
            {
                Active_Sx_Prevalence[age_group] += mc_weight;

                if (iptrtb->IsSmearPositive())
                {
                    Active_Smear_Positive[age_group] += mc_weight;
                }
            }
        }

        if (iptrtb->HasLatentInfection())
        {
            Latent[age_group] += mc_weight;
        }

        if (iptrhiv->HasHIV())
        {

            HIVstatus[age_group] += mc_weight;
            if (iptrhiv->GetHIVInterventionsContainer()->OnArtQuery())
            {
                OnART[age_group] += mc_weight;
            }
        }

    }

    bool Report_TBHIV_ByAge::notifyOnEvent( IIndividualHumanEventContext *context, 
                                            const EventTrigger& trigger )
    {
        if( context->GetAge() < min_age_yrs * DAYSPERYEAR || context->GetAge() > max_age_yrs * DAYSPERYEAR)
            return true;

        // iindividual context for suid
        IIndividualHumanContext * iindividual = NULL;
        if (s_OK != context->QueryInterface(GET_IID(IIndividualHumanContext), (void**)&iindividual) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanContext", "IIndividualHumanEventContext");
        }

        IIndividualHumanHIV * iptrhiv = NULL;
        if (s_OK != iindividual->GetEventContext()->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&iptrhiv))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "GetEventContext", "IIndividualHumanHIV", "IIndividualHumanEventContext");
        }

        IIndividualHumanTB * iptrtb = NULL;
        if (s_OK != iindividual->GetEventContext()->QueryInterface(GET_IID(IIndividualHumanTB), (void**)&iptrtb))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "GetEventContext", "IIndividualHumanTB", "IIndividualHumanEventContext");
        }

        float mc_weight = context->GetMonteCarloWeight();

        auto age_group = ComputeAgeBin((float)context->GetAge());


        if( trigger == EventTrigger::TBActivation )
        {
            New_Activations[age_group] += mc_weight;
            if (iptrhiv->HasHIV())
            {
                HIVPosNewActivations[age_group] += mc_weight;
            }
            if (iptrtb->IsMDR())
            {
                IncidentMDR[age_group] += mc_weight;
            }
        }
        else if( trigger == EventTrigger::DiseaseDeaths )
        {
            DiseaseDeaths[age_group] += mc_weight;
            if (iptrhiv->HasHIV() )
            {
                HIVPosTBDeaths[age_group] += mc_weight;
            }
        }
        else if( trigger == EventTrigger::NonDiseaseDeaths )
        {
            NonDiseaseDeaths[age_group] += mc_weight;       
        }
        else if (trigger == EventTrigger::OpportunisticInfectionDeath)
        {
            HIVDeaths[age_group] += mc_weight;
            if (iptrtb->HasActiveInfection())
            {
                HIVDeathsActiveTB[age_group] += mc_weight;
            }
        }
        else if (trigger == EventTrigger::Births)
        {
            Births += mc_weight;
        }
        else if (trigger == EventTrigger::TBStartDrugRegimen)
        {
            TBStartTreatment[age_group] += mc_weight;
        }
        else if (trigger == EventTrigger::TBFailedDrugRegimen)
        {
            TBFailedTreatment[age_group] += mc_weight;
        }
        else if (trigger == EventTrigger::NewInfectionEvent)
        {
            NewInfections[age_group] += mc_weight; 
        }
        else if (trigger == EventTrigger::TBTestPositive)
        {
            PotentialNotifications[age_group] += mc_weight;
            if (iptrtb->HasFailedTreatment())
            {
                Retreatments[age_group] += mc_weight;
            }
            if (iptrhiv->HasHIV())
            {
                HIVPosNotifications[age_group] += mc_weight;
            }
        }
        else if (trigger == EventTrigger::ProviderOrdersTBTest)
        {
            TBTests[age_group] += mc_weight;
        }
        else
        {
            for (vector<string>::iterator it = Additional_Event_Names.begin(); it != Additional_Event_Names.cend(); ++it)
            {
                int index = std::distance(Additional_Event_Names.begin(), it);
                if (trigger == EventTrigger(*it))
                {
                    DynamicEvents[index][age_group] += mc_weight;
                }

            }
            
        }

        return true;
    }

}