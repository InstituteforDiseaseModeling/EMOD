/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ReportVector.h"
#include "DengueDefs.h" // for N_DENGUE_SEROTYPES
#include "SimulationEnums.h" // for DengueVirusTypes
#include "SusceptibilityDengue.h" // for DengueVirusTypes
#include <map>

namespace Kernel {

class ReportDengue : public ReportVector
{
    GET_SCHEMA_STATIC_WRAPPER(ReportDengue)
public:
    ReportDengue();
    virtual ~ReportDengue() {};

    static IReport* ReportDengue::CreateReport() { return new ReportDengue(); }

    virtual bool Configure( const Configuration * inputJson );
    virtual void EndTimestep( float currentTime, float dt );

    float getAcqMod( ISusceptibilityDengue* dengue_immunity, unsigned int strainId );
    virtual void LogIndividualData( IIndividualHuman * individual);
    virtual void LogNodeData( Kernel::INodeContext * pNC );

protected:
    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
    virtual void postProcessAccumulatedData();

    static const char* naive_immune_pop_label;
    static const char* primary_immune_pop_label;
    static const char* secondary_immune_pop_label;
    static const char* postsec_immune_pop_label;

    unsigned int new_infections_by_strain[ 4 ];
    unsigned int new_reported_infections_by_strain[ 4 ];

private:
};

}
