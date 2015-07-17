/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "BaseEventReport.h"


namespace Kernel
{
    // This base class extends BaseEventReport by adding code to EndTimestep()
    // so that the output data is written periodically.  The user can control
    // how often the data is written using the "internval" parameters in the
    // configuration data.  In addition, this type of report can also control
    // how many reports will be created.  Once the max has been created, the
    // report will stop collecting data.
    class IDMAPI BaseEventReportIntervalOutput : public Kernel::BaseEventReport
    {
    public:

        BaseEventReportIntervalOutput( const std::string& rReportName );
        virtual ~BaseEventReportIntervalOutput();

        // BaseEventReport
        virtual bool Configure( const Configuration* );
        virtual void EndTimestep( float currentTime, float dt );
        virtual void Finalize();

    protected:
        // method for subclass to override/implement
        virtual void ClearOutputData() {} ;
        virtual void WriteOutput( float currentTime ) {} ;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        float  m_interval_timer;
        float  m_reporting_interval;
        int  m_report_count;
        int  m_max_number_reports;
#pragma warning( pop )
    };

};