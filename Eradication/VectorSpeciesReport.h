/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BinnedReport.h"

namespace Kernel {

class VectorSpeciesReport : public BinnedReport
{
public:
    static IReport* CreateReport( const Kernel::jsonConfigurable::tDynamicStringSet& rVectorSpeciesNames );
    virtual ~VectorSpeciesReport();

    virtual void Initialize( unsigned int nrmSize );

    virtual void LogNodeData( Kernel::INodeContext * pNC );
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const { return false ; } ;
    virtual void LogIndividualData( Kernel::IIndividualHuman* individual );
    virtual void EndTimestep( float currentTime, float dt );

protected:
    VectorSpeciesReport( const Kernel::jsonConfigurable::tDynamicStringSet& rVectorSpeciesNames );

    virtual void initChannelBins();
    void clearChannelsBins();
    virtual void postProcessAccumulatedData();
    void normalizeChannelWithLastTimestep( const std::string &channel_name, const std::string &normalization_channel_name );

    // channels specific to this particular report-type
    float* adult_vectors;
    float* infectious_vectors;
    float* daily_eir;
    float* daily_hbr;
    float* dead_vectors_before;
    float* dead_vectors_indoor;
    float* dead_vectors_outdoor;
};

} 
