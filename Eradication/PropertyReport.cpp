
#include "stdafx.h"

#include "IdmString.h"
#include "IIndividualHuman.h"
#include "PropertyReport.h"
#include "Properties.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "PropertyReport" )

static const std::string _report_name = "PropertyReport.json";

namespace Kernel {

const char * PropertyReport::_pop_label = "Statistical Population:";
const char * PropertyReport::_infected_label = "Infected:";
const char * PropertyReport::_new_infections_label = "New Infections:";
const char * PropertyReport::_disease_deaths_label = "Disease Deaths:";

/////////////////////////
// Initialization methods
/////////////////////////
IReport*
PropertyReport::CreateReport()
{
    return new PropertyReport( _report_name );
}

PropertyReport::PropertyReport( const std::string& rReportName )
: BaseChannelReport( rReportName )
, permutationsList()
, new_infections()
, disease_deaths()
, infected()
, statPop()
{
}

void PropertyReport::Initialize( unsigned int nrmSize )
{
    permutationsList = IPFactory::GetInstance()->GetAllPossibleKeyValueCombinations<IPKeyValueContainer>();
}

void PropertyReport::populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map )
{
}

void
PropertyReport::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    std::string reportingBucket = individual->GetProperties()->ToString();

    float monte_carlo_weight = (float)individual->GetMonteCarloWeight();
    NewInfectionState::_enum nis = individual->GetNewInfectionState();

    if(nis == NewInfectionState::NewAndDetected || nis == NewInfectionState::NewInfection)
        new_infections[ reportingBucket ] += monte_carlo_weight;

    //if(individual->GetStateChange() == HumanStateChange::KilledByInfection)
    //    disease_deaths[ reportingBucket ] += monte_carlo_weight;

    if (individual->IsInfected())
    {
        infected[ reportingBucket ] += monte_carlo_weight;
    }

    statPop[ reportingBucket ] += monte_carlo_weight;
}

void
PropertyReport::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    LOG_DEBUG( "LogNodeData\n" );
    for (auto& reportingBucket : permutationsList)
    {
        Accumulate(_new_infections_label + reportingBucket, new_infections[reportingBucket]);
        new_infections[reportingBucket] = 0.0f;
        //Accumulate(_disease_deaths_label + reportingBucket, disease_deaths[reportingBucket]);
        Accumulate(_pop_label + reportingBucket, statPop[ reportingBucket ] );
        statPop[reportingBucket] = 0.0f;
        Accumulate(_infected_label + reportingBucket, infected[ reportingBucket ]);
        infected[ reportingBucket ] = 0.0f;
    }
}

// normalize by time step and create derived channels
void
PropertyReport::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData in PropertyReport\n" );

}

// This is just to avoid Disease Deaths not broken down by props which is in base class. Why is it there?
void
PropertyReport::EndTimestep( float currentTime, float dt )
{
    LOG_DEBUG( "EndTimestep\n" );
}

};
