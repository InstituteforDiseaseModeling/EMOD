/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <sstream>
#include "SimulationEnums.h"
#include "BaseTextReportEvents.h"

#define MAX_AGE (100)

namespace Kernel {
    struct ISimulation;

    namespace Yes_No { 
        enum Yes_No { YES, NO, COUNT };
    }

    class ReportHIVByAgeAndGender : public BaseTextReportEvents
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportHIVByAgeAndGender)
    public:
        ReportHIVByAgeAndGender( const ISimulation *sim = nullptr, float hivPeriod = 180.0 );
        static IReport* ReportHIVByAgeAndGender::Create(const ISimulation * parent, float hivPeriod) { return new ReportHIVByAgeAndGender( parent, hivPeriod ); }

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual bool Configure( const Configuration* inputJson );
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList );
        virtual std::string GetHeader() const ;
        virtual bool notifyOnEvent(IIndividualHumanEventContext *context, const std::string& StateChange);

        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const ;
        virtual void LogIndividualData( IndividualHuman* individual );
        virtual void LogNodeData( INodeContext* pNC );

    protected:

    private:

        const float report_hiv_half_period;
        float next_report_time;
        bool doReport;

        float population[Gender::Enum::COUNT][MAX_AGE];         //Gender, Age --> Population
        float infected[Gender::Enum::COUNT][MAX_AGE];           //Gender, Age --> Infected
        float newly_infected[Gender::Enum::COUNT][MAX_AGE];     //Gender, Age --> Newly Infected
        float on_ART[Gender::Enum::COUNT][MAX_AGE];             //Gender, Age --> On ART
        float newly_died[Gender::Enum::COUNT][MAX_AGE];         //Gender, Age --> Newly Died
        float newly_died_from_HIV[Gender::Enum::COUNT][MAX_AGE];//Gender, Age --> Newly Died from HIV
        
        float tested_ever[Gender::Enum::COUNT][MAX_AGE];        //Gender, Age --> Tested ever
        float tested_past_year_or_onART[Gender::Enum::COUNT][MAX_AGE];   //Gender, Age --> Tested past year (or on ART)


        const ISimulation * _parent;
        float startYear ;                                       // Year to start collecting data
        float stopYear ;                                        // Year to stop  collecting data
        bool is_collecting_data ;
    };

}

