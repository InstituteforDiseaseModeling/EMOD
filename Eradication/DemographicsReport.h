
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

    ChannelID avg_age_id;
    ChannelID gender_id;
    ChannelID pseudo_pop_id;
    ChannelID births_id;
    ChannelID deaths_id;
    ChannelID stat_pop_id;
    ChannelID mothers_id;
    std::vector<ChannelID> pop_age_ids;

    static float age_buckets[21];
    static std::string age_ranges[(sizeof(age_buckets) / sizeof(float)) + 1];

    float population_by_age_bucket[(sizeof(age_buckets) / sizeof(float)) + 1];
};
