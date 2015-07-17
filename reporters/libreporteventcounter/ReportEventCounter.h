/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <map>

#include "BaseEventReport.h"
#include "ChannelDataMap.h"

namespace Kernel
{
    class ReportEventCounter : public BaseEventReport
    {
    public:
        ReportEventCounter();
        virtual ~ReportEventCounter();

        // BaseEventReport
        virtual bool Configure( const Configuration* );
        virtual void BeginTimestep() ;
        virtual void Reduce();
        virtual void Finalize();
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const std::string& StateChange );
    private:
        ChannelDataMap channelDataMap ;
        std::map<std::string, std::string> unitsMap;
    };
}
