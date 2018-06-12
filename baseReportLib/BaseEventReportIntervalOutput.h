/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseEventReport.h"


namespace Kernel
{
    struct IJsonObjectAdapter;
    class JSerializer;

    // This is an interface to an object that is used to collect that data for
    // a particular reporting interval.  The data in this object is expected
    // to be sent to the Rank=0 core and accumulated there.
    struct IIntervalData
    {
        virtual void Clear() = 0;
        virtual void Update( const IIntervalData& rOther ) = 0;
        virtual void Serialize( IJsonObjectAdapter& rjoa, JSerializer& js ) = 0;
        virtual void Deserialize( IJsonObjectAdapter& rjoa ) = 0;
    };


    // This base class extends BaseEventReport by adding code to EndTimestep()
    // so that the output data is written periodically.  The user can control
    // how often the data is written using the "internval" parameters in the
    // configuration data.  In addition, this type of report can also control
    // how many reports will be created.  Once the max has been created, the
    // report will stop collecting data.
    class IDMAPI BaseEventReportIntervalOutput : public Kernel::BaseEventReport
    {
    public:

        BaseEventReportIntervalOutput();
        BaseEventReportIntervalOutput( const std::string& rReportName, 
                                       bool oneFilePerReport, 
                                       IIntervalData* pIntervalData,
                                       IIntervalData* pMulticoreDataExchange );
        virtual ~BaseEventReportIntervalOutput();

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void Reduce() override;
        virtual void Finalize() override;

    protected:
        // method for subclass to override/implement
        virtual void AccumulateOutput() {};
        virtual void ClearOutputData();
        virtual void SerializeOutput( float currentTime, IJsonObjectAdapter& output, JSerializer& js ) {};
        virtual std::string CreateOutputFilename( float currentTime );
        virtual void WriteOutput( float currentTime );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        float  m_current_time;
        float  m_interval_timer;
        float  m_reporting_interval;
        int  m_report_count;
        int  m_max_number_reports;
        bool m_one_file_per_report;
        bool m_has_data;
        bool m_expired;
        bool m_PrettyFormat;
        IIntervalData* m_pIntervalData;
        IIntervalData* m_pMulticoreDataExchange;
#pragma warning( pop )
    };

};