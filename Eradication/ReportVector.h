/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Report.h"

namespace Kernel {

class ReportVector : public Report
{
public:
    ReportVector();
    virtual ~ReportVector() {};

    static IReport* ReportVector::CreateReport() { return new ReportVector(); }

    virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

protected:
    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
    virtual void postProcessAccumulatedData() override;

    virtual void AddSEIRWUnits( std::map<std::string, std::string> &units_map ) override {}
    virtual void UpdateSEIRW( const Kernel::IIndividualHuman* individual, float monte_carlo_weight ) override {}
    virtual void AccumulateSEIRW() override {}
    virtual void NormalizeSEIRWChannels() override {}
};

}
