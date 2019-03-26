/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <sstream>
#include "SimulationEnums.h"
#include "BaseTextReportEvents.h"
#include "PropertyReport.h" // for some types and a (static) function

#define MAX_AGE (100)

namespace Kernel {
    struct ISimulation;

    class ReportTyphoidByAgeAndGender : public BaseTextReportEvents
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportTyphoidByAgeAndGender)
    public:
        ReportTyphoidByAgeAndGender( const ISimulation *sim = nullptr, float period = 180.0 );
        static IReport* ReportTyphoidByAgeAndGender::Create(const ISimulation * parent, float period) { return new ReportTyphoidByAgeAndGender( parent, period ); }

        // -----------------------------
        // --- BaseTextReportEvents
        // -----------------------------
        virtual bool Configure( const Configuration* inputJson );
        virtual void Initialize( unsigned int nrmSize ) override;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext );
        virtual std::string GetHeader() const ;
        virtual bool notifyOnEvent(IIndividualHumanEventContext *context, const EventTrigger& StateChange);

        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const ;
        virtual void LogIndividualData( IIndividualHuman* individual );
        virtual void LogNodeData( INodeContext* pNC );

    protected:
        //typedef std::map< std::string, std::string > tKeyValuePair; // pairs?
        //typedef std::set< tKeyValuePair > tPermutations;
        PropertyReport::tPermutations permutationsSet;
        std::vector<std::string> permutationsList ;

    private:

        //const float report_half_period;
        NonNegativeFloat next_report_time;
        bool doReport;
#define MAX_PROPS 10 // figure this out
        float population[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];         //Gender, Age --> Population
        float infected[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           //Gender, Age --> Infected
        float newly_infected[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];     //Gender, Age --> Newly Infected 

        float chronic[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           
        float subClinical[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           
        float acute[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           
        float prePatent[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           

        float chronic_inc[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           
        float subClinical_inc[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           
        float acute_inc[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           
        float prePatent_inc[Gender::Enum::COUNT][MAX_AGE][ MAX_PROPS ];           

        const ISimulation * _parent;
        float startYear ;                                       // Year to start collecting data
        float stopYear ;                                        // Year to stop  collecting data
        bool is_collecting_data ;

        std::map< std::string, int > bucketToIdMap;
    };

}

