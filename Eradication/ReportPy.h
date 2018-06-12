/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Report.h"
#include "SimulationEnums.h" // for PyVirusTypes
#include "TransmissionGroupMembership.h"
#include <map>

namespace Kernel {

class ReportPy : public Report
{
    GET_SCHEMA_STATIC_WRAPPER(ReportPy)
public:
    ReportPy();
    virtual ~ReportPy() {};

    static IReport* ReportPy::CreateReport() { return new ReportPy(); }

    virtual bool Configure( const Configuration * inputJson );
    virtual void EndTimestep( float currentTime, float dt );

    virtual void LogIndividualData( IIndividualHuman * individual);
    virtual void LogNodeData( INodeContext * pNC );

protected:
    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
    virtual void postProcessAccumulatedData();

private:

    //TransmissionGroupMembership_t memberships;

};

}
