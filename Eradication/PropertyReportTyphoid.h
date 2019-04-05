/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "PropertyReportEnvironmental.h"
//#include "IndividualTyphoid.h"

namespace Kernel {

    class PropertyReportTyphoid : public PropertyReportEnvironmental
    {
        GET_SCHEMA_STATIC_WRAPPER( PropertyReportTyphoid )
    public:
        static IReport* CreateReport();
        virtual ~PropertyReportTyphoid() { }

        virtual bool Configure( const Configuration* inputJson ) override;
        virtual void UpdateEventRegistration( float currentTime,
                                              float dt,
                                              std::vector<INodeEventContext*>& rNodeEventContextList,
                                              ISimulationEventContext* pSimEventContext ) override;
        virtual void BeginTimestep() override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override;
        virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

    protected:
        PropertyReportTyphoid();
        float start_year; // Year to start collecting data
        float stop_year;  // Year to stop  collecting data 
        bool is_collecting_data;
    };

#if 0
    template<class Archive>
    void serialize(Archive &ar, PropertyReport& report, const unsigned int v)
    {
        boost::serialization::void_cast_register<PropertyReport,IReport>();
        //ar & report.timesteps_reduced;
        //ar & report.channelDataMap;
        //ar & report._nrmSize;
    }
#endif

}
