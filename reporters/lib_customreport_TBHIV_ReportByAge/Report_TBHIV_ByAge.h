/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseTextReportEvents.h"
#include "SimulationEnums.h"
#include "Properties.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"


namespace Kernel
{
  
	ENUM_DEFINE(ARTStatusLocal,
	    ENUM_VALUE_SPEC(NA  ,       0)
		ENUM_VALUE_SPEC(OFFART  ,      1)
		ENUM_VALUE_SPEC(ONART   ,       2)
		ENUM_VALUE_SPEC(COUNT   ,    3))

   ENUM_DEFINE(TB_State, 
	   ENUM_VALUE_SPEC(NA                     ,0)
	   ENUM_VALUE_SPEC(Negative               ,1)
	   ENUM_VALUE_SPEC(Latent                 ,2)
	   ENUM_VALUE_SPEC(ActivePreSymptomatic   ,3)
	   ENUM_VALUE_SPEC(ActiveSmearPos         , 4)
	   ENUM_VALUE_SPEC(ActiveSmearNeg         , 5)
	   ENUM_VALUE_SPEC(ActiveExtraPulm        , 6)
       ENUM_VALUE_SPEC(COUNT                  , 7))

   ENUM_DEFINE(MDR_State, 
	   ENUM_VALUE_SPEC(NA                     , 0)
	   ENUM_VALUE_SPEC(Negative               , 1)
	   ENUM_VALUE_SPEC(MDR                    , 2)
	   ENUM_VALUE_SPEC(COUNT                  , 3)
   )

   //Note naming is short for less than stated year and greater than previous cateogory ( ie LESS_5 is 1 yr <= age <5 yrs) 
   
      ENUM_DEFINE(Report_Age,     
            ENUM_VALUE_SPEC(LESS_1, 0)  
            ENUM_VALUE_SPEC(LESS_5, 1)
            ENUM_VALUE_SPEC(LESS_10, 2)
            ENUM_VALUE_SPEC(LESS_15, 3)
            ENUM_VALUE_SPEC(LESS_20, 4)
            ENUM_VALUE_SPEC(LESS_25, 5)
            ENUM_VALUE_SPEC(LESS_30, 6)
            ENUM_VALUE_SPEC(LESS_35, 7)
            ENUM_VALUE_SPEC(LESS_40, 8)
            ENUM_VALUE_SPEC(LESS_45, 9)
            ENUM_VALUE_SPEC(LESS_50, 10)
            ENUM_VALUE_SPEC(LESS_55, 11)
            ENUM_VALUE_SPEC(LESS_60, 12)
            ENUM_VALUE_SPEC(LESS_65, 13)
            ENUM_VALUE_SPEC(LESS_70, 14)
            ENUM_VALUE_SPEC(LESS_75, 15)
            ENUM_VALUE_SPEC(LESS_80, 16)
            ENUM_VALUE_SPEC(LESS_85, 17)
            ENUM_VALUE_SPEC(LESS_90, 18)
            ENUM_VALUE_SPEC(LESS_95, 19)
            ENUM_VALUE_SPEC(GREAT_95, 20)
            ENUM_VALUE_SPEC(COUNT, 21      )  )   // Needed for array initialization below


    class Report_TBHIV_ByAge : public BaseTextReportEvents
    {
    public:
        Report_TBHIV_ByAge();
        virtual ~Report_TBHIV_ByAge();

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;
        virtual void Initialize( unsigned int nrmSize ) override;
        //virtual void BeginTimestep() ;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList ) override;

        virtual std::string GetHeader() const override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
        //virtual void Reduce();
        //virtual void Finalize();
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override;
    private:

        const float report_tbhiv_half_period;
        float next_report_time;
        bool doReport;
        float startYear;
        float stopYear;
        bool is_collecting_data;
        float min_age_yrs;
        float max_age_yrs;

        Report_Age::Enum ComputeAgeBin(float age);

        float Population[Report_Age::Enum::COUNT];
        float DiseaseDeaths[Report_Age::Enum::COUNT];
        float NonDiseaseDeaths[Report_Age::Enum::COUNT];
        float OnART[Report_Age::Enum::COUNT];     
        float New_Activations[Report_Age::Enum::COUNT];                                                 //                       --> Infections
        float Active_Prevalence[Report_Age::Enum::COUNT];
        float Active_Sx_Prevalence[Report_Age::Enum::COUNT];
        float Active_PreSymptomatic[Report_Age::Enum::COUNT];
        float Active_Smear_Positive[Report_Age::Enum::COUNT];
        float Latent[Report_Age::Enum::COUNT];
        float HIVstatus[Report_Age::Enum::COUNT];
        float Births;
        float HIVDeaths[Report_Age::Enum::COUNT];
        float TBStartTreatment[Report_Age::Enum::COUNT];
        
        float TBFailedTreatment[Report_Age::Enum::COUNT];
        float PotentialNotifications[Report_Age::Enum::COUNT];
        float Retreatments[Report_Age::Enum::COUNT];
        float PrevalentMDR[Report_Age::Enum::COUNT];
        float IncidentMDR[Report_Age::Enum::COUNT];
        float NewInfections[Report_Age::Enum::COUNT];
        float HIVPosNewActivations[Report_Age::Enum::COUNT];
        float HIVPosTBDeaths[Report_Age::Enum::COUNT];
        float HIVPosNotifications[Report_Age::Enum::COUNT];
        float TBTests[Report_Age::Enum::COUNT];
        float HIVDeathsActiveTB[Report_Age::Enum::COUNT];

        vector <std::string> Additional_Event_Names;
        float DynamicEvents[100][Report_Age::Enum::COUNT];
                                                                           
    };
}
