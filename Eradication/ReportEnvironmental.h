/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Report.h"

namespace Kernel
{
    class ReportEnvironmental : public Report
    {
    public:
        ReportEnvironmental();
        virtual ~ReportEnvironmental() {};

        static IReport* ReportEnvironmental::CreateReport() { return new ReportEnvironmental(); }
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual void LogIndividualData( IIndividualHuman * individual) override;
        virtual void LogNodeData( INodeContext * pNC ) override;

    protected:

        virtual void AddSEIRWUnits( std::map<std::string, std::string> &units_map ) override {}
        virtual void UpdateSEIRW( const IIndividualHuman* individual, float monte_carlo_weight ) override {}
        virtual void AccumulateSEIRW() override {}
        virtual void NormalizeSEIRWChannels() override {}

    private:
        NonNegativeFloat enviro_infections_counter;
        NonNegativeFloat contact_infections_counter;
    };
}
