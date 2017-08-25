/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#pragma once

#include "BaseTextReportEvents.h"

namespace Kernel
{
    class ReportHumanMigrationTracking : public BaseTextReportEvents
    {
    public:
        ReportHumanMigrationTracking();
        virtual ~ReportHumanMigrationTracking();

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList ) override;

        virtual std::string GetHeader() const override;
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const EventTrigger& trigger ) override;
        virtual void LogIndividualData( IIndividualHuman* individual ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void Reduce() override;
    private:
        struct MigrationData
        {
            MigrationData() : age_years(-1.0), gender(-1), home_node_id(-1), node_id(-1), migration_type_str() {}

            float age_years ;
            int gender ;
            bool is_adult ;
            uint32_t home_node_id ;
            uint32_t node_id ;
            std::string migration_type_str ;
        };
        float m_EndTime ;
        std::map<long,MigrationData> m_MigrationDataMap ;
    };
}
