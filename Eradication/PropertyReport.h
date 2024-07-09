
#pragma once

#include "BaseChannelReport.h"

/*
Flexible aggregator for simulation-wide numeric values. Intended to serve as the mechanism for reporting inset charts.
The class interface and usage pattern is customized for computing, for each timestep, a sum (accumulate) of a labeled quantity 
across the simulation.

Accumulate values into the sum for the current timestep with a call to Accumulate("ChannelName", value).
Wrap each timestep's accumulation events in calls to Begin/EndTimestep().
All the channels that will ever be accumulated into need to have had Accumulate() called for them before the first EndTimestep(),
or unspecified behavior may result later.

To finalize the accumulation across distributed MPI processes, call Reduce(); data will be valid on rank 0 only.
*/
namespace Kernel {
    class IndividualHuman; // fwd declaration
    class PropertyReport : public BaseChannelReport
    {
    public:
        static IReport* CreateReport();
        virtual ~PropertyReport() { }

        virtual void Initialize(unsigned int nrmSize) override;
        virtual void EndTimestep( float currentTime, float dt ) override;
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const override { return true; };
        virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

    protected:
        PropertyReport( const std::string& rReportName );

        std::vector<std::string> permutationsList;

        virtual void postProcessAccumulatedData() override;
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map ) override;

        // counters
        std::map< std::string, float > new_infections;
        std::map< std::string, float > disease_deaths;
        std::map< std::string, float > infected;
        std::map< std::string, float > statPop;

        //labels
        static const char * _pop_label;
        static const char * _infected_label;
        static const char * _new_infections_label;
        static const char * _disease_deaths_label;
    };

};

#if 0
template<class Archive>
void serialize(Archive &ar, PropertyReport& report, const unsigned int v)
{
    //ar & report.timesteps_reduced;
    //ar & report.channelDataMap;
    //ar & report._nrmSize;
}
#endif
