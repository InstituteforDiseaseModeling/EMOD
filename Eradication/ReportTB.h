/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "stdafx.h"

#include "ReportAirborne.h"

namespace Kernel
{
    class ReportTB : public ReportAirborne
    {
    public:
        ReportTB();
        virtual ~ReportTB(){};

        static IReport* ReportTB::CreateReport() { return _new_ ReportTB(); }

        virtual void BeginTimestep() override;
        virtual void EndTimestep( float currentTime, float dt ) override;

        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;
        virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;

    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;
        virtual void postProcessAccumulatedData() override;

        virtual void UpdateSEIRW( const Kernel::IIndividualHuman * individual, float monte_carlo_weight ) override;

        //counters
        float latent_TB_persons;
        float latent_fast;
        float active_TB_persons;
        float active_presymptomatic;
        float active_smear_neg;
        float active_sx_smear_pos;
        float active_sx_smear_neg;
        float active_sx_extrapulm;
        
        float MDR_TB_persons;
        float active_MDR_TB_persons;
        float TB_immune_persons;

        float new_active_TB_infections;
        float newly_cleared_TB_infections;
        float new_smear_positive_infections;
        float new_active_fast_TB_infections;
        float new_active_slow_TB_infections;
        float new_mdr_active_infection;
        float mdr_evolved_incident_counter;
        float new_mdr_fast_active_infection_counter;

        float disease_deaths_MDR;

        float active_sx;
        float mdr_active_sx;
        float mdr_active_sx_smear_pos;
        float mdr_active_sx_evolved;

        float infectiousness_fast;
    };
}
