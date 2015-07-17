/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "BaseChannelReport.h"
#include "IndividualEventContext.h"
#include "Interventions.h"

namespace Kernel{

class Report_Scenarios : public BaseChannelReport, public IIndividualEventObserver
{
    IMPLEMENT_DEFAULT_REFERENCE_COUNTING();

public:
    Report_Scenarios();
    virtual ~Report_Scenarios();

protected:
    virtual void LogNodeData( INodeContext * pNC );
    virtual bool IsCollectingIndividualData( float currentTime, float dt ) const { return true ; } ;
    virtual void LogIndividualData( IndividualHuman * individual );
    virtual void BeginTimestep();
    virtual void EndTimestep( float currentTime, float dt );
    virtual void Finalize();

    // for IIndividualEventObserver
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
    virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const {}; //GHH weird I had to add these?
    virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper ) {}; //GHH weird I had to add these?

    virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
    virtual void postProcessAccumulatedData();
    
    virtual int calcBinIndex(const IndividualHuman * individual);

    // for INodeEventObserver
    virtual bool notifyOnEvent( IIndividualHumanEventContext *context, const std::string& StateChange );

    int last_years_births;
    int this_years_births;

private:
    const int _countupToNextPeriodicTarget;
    float countupToNextPeriodicReport;
    std::vector<INodeTriggeredInterventionConsumer*> ntic_list ;
};

}
