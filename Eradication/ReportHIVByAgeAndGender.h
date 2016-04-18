/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <sstream>
#include "SimulationEnums.h"
#include "BaseTextReportEvents.h"

#define MAX_AGE (100)

namespace Kernel {
    struct ISimulation;

    namespace Yes_No { 
        enum Yes_No { NO, YES, COUNT };
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
        virtual void Initialize( unsigned int nrmSize );
        virtual std::string GetHeader() const ;
        virtual bool notifyOnEvent(IIndividualHumanEventContext *context, const std::string& StateChange);

        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const ;
        virtual void LogIndividualData( IIndividualHuman* individual );
        virtual void LogNodeData( INodeContext* pNC );
        virtual void EndTimestep(float currentTime, float dt);

    protected:

    private:

        const float report_hiv_half_period;
        float next_report_time;
        bool doReport;

        struct ReportData
        {
            ReportData()
                : population(0.0)
                , infected(0.0)
                , infected_noART_cd4_under_200(0.0)
                , infected_noART_cd4_200_to_350(0.0)
                , infected_noART_cd4_350_to_500(0.0)
                , infected_noART_cd4_above_500(0.0)
                , newly_infected(0.0)
                , tested_positive(0.0)
                , tested_negative(0.0)
                , on_ART(0.0)
                , newly_died(0.0)
                , newly_died_from_HIV(0.0)
                , tested_ever_HIVpos(0.0)
                , tested_ever_HIVneg(0.0)
                , tested_past_year_or_onART(0.0)
                , event_counter_map()
            {
            }

            float population;                // Population
            float infected;                  // Infected

            // The following are used when Report_HIV_ByAgeAndGender_Stratify_Infected_By_CD4 is set 
            // N.B. This is infected _and_ not on ART by CD4
            float infected_noART_cd4_under_200;
            float infected_noART_cd4_200_to_350;
            float infected_noART_cd4_350_to_500;
            float infected_noART_cd4_above_500;

            float newly_infected;            // Newly Infected
            float tested_positive;           // Newly Tested Positive
            float tested_negative;           // Newly Tested Negative
            float on_ART;                    // On ART
            float newly_died;                // Newly Died
            float newly_died_from_HIV;       // Newly Died from HIV
            float tested_ever_HIVpos;        // Tested ever [amongst HIV+]
            float tested_ever_HIVneg;        // Tested ever [amongst HIV-]
            float tested_past_year_or_onART; // Tested past year (or on ART)

            std::map<std::string,float> event_counter_map; // count the ocurrences of events
        };

        bool GetNextIP( std::vector<int>& rKeyValueIndexList );
        uint32_t GetDataMapKey( IIndividualHumanEventContext* context );
        uint32_t GetDataMapKey( int nodeSuidIndex, int genderIndex, int ageIndex, int circIndex, const std::vector<int>& rKeyValueIndexList );
        void AddConstant();

        std::map<uint32_t,ReportData> data_map ;

        const ISimulation * _parent;
        float startYear ;                                       // Year to start collecting data
        float stopYear ;                                        // Year to stop  collecting data
        bool is_collecting_data ;
        bool is_collecting_circumcision_data;
        bool is_collecting_ip_data;
        bool stratify_infected_by_CD4;
        std::vector<std::string> event_list;
        std::vector<std::string> ip_key_list ;
        std::map<std::string,std::vector<std::string>> ip_key_value_list_map ;
        std::vector<uint32_t> map_key_constants ;
    };

}

