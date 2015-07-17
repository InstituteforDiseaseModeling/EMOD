/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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

    virtual void LogNodeData( Kernel::INodeContext * pNC );

protected:
    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
    virtual void postProcessAccumulatedData();

    virtual void AddSEIRWUnits( std::map<std::string, std::string> &units_map ) {}
    virtual void UpdateSEIRW( const Kernel::IndividualHuman * individual, float monte_carlo_weight ) {}
    virtual void AccumulateSEIRW() {}
    virtual void NormalizeSEIRWChannels() {}

private:
#if USE_BOOST_SERIALIZATION
    friend class ::boost::serialization::access;
    template<class Archive>
    friend void serialize(Archive &ar, ReportVector& report, const unsigned int v);
#endif
};

}
