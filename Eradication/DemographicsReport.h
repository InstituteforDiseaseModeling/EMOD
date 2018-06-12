/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseChannelReport.h"

class DemographicsReport : public BaseChannelReport
{
public:
    static IReport* CreateReport();
    virtual ~DemographicsReport() { }

    virtual void Initialize( unsigned int nrmSize );

    virtual void BeginTimestep();
    virtual void LogNodeData( Kernel::INodeContext * pNC );
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const { return true ; } ;
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual );
    virtual void EndTimestep( float currentTime, float dt );

protected:
    DemographicsReport();

    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) { }
    virtual void postProcessAccumulatedData();

    int   pseudoPop;
    float totalAge;
    float totalMales;
    float oldBirths, totalBirths;
    float newNaturalDeaths;

    static float age_buckets[20];
    static std::string age_ranges[(sizeof(age_buckets) / sizeof(float)) + 1];

    float population_by_age_bucket[(sizeof(age_buckets) / sizeof(float)) + 1];
};
