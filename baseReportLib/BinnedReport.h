/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BaseChannelReport.h"
#include "IIndividualHuman.h"

namespace json {
    class Element;
}

namespace Kernel {
    struct IJsonObjectAdapter;

class BinnedReport : public BaseChannelReport
{
public:
    static IReport* CreateReport();
    virtual ~BinnedReport();

    virtual void Initialize( unsigned int nrmSize );

    virtual void BeginTimestep();
    virtual void LogNodeData( Kernel::INodeContext * pNC );
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const { return true ; } ;
    virtual void LogIndividualData( IIndividualHuman* individual );
    virtual void EndTimestep( float currentTime, float dt );

    virtual void Finalize();

protected:
    BinnedReport();
    BinnedReport( const std::string& rReportName );

    virtual void Accumulate(std::string channel_name, float bin_data[]);
    virtual void Accumulate(std::string channel_name, float value, int bin_index);
    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
    virtual void postProcessAccumulatedData();

    virtual void initChannelBins();
    void clearChannelsBins();
    virtual int calcBinIndex( IIndividualHuman* individual);

    // TODO: should return-type be something generic (void* ?) so e.g. MATLAB plugin can follow this pattern?
    virtual json::Element formatChannelDataBins(const float data[], std::vector<int>& dims, int start_axis, int num_remaining_bins);
    void formatChannelDataBins(Kernel::IJsonObjectAdapter* pIJsonObj, const float data[], std::vector<int>& dims, int start_axis, int num_remaining_bins);

    // BaseChannelReport
    virtual void SetAugmentor( IChannelDataMapOutputAugmentor* pAugmentor ) { p_output_augmentor = pAugmentor; };

    // general sub-channel binning
    int num_timesteps;
    int num_axes;
    std::vector<std::string> axis_labels;
    std::vector<int> num_bins_per_axis;
    int num_total_bins;
    std::vector<std::vector<float> > values_per_axis;
//    std::vector<std::vector<std::string> > friendly_names_per_axis;

    //labels (these are in the header instead of the cpp file so they can be inherited by disease specific binned report
    static const char * _pop_label;
    static const char * _infected_label;
    static const char * _new_infections_label;
    
    // channels specific to this particular report-type
    float *population_bins;
    float *infected_bins;
    float *new_infections_bins;
    
    IChannelDataMapOutputAugmentor* p_output_augmentor ;
    int _num_age_bins;
    int _num_bins_per_axis[1];
    float * _age_bin_upper_values;
    std::vector<std::string> _age_bin_friendly_names;
                                  
};

}
