/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "IIndividualHuman.h"
#include "Node.h"
#include "PropertyReportEnvironmental.h"
#include "TransmissionGroupsBase.h"
#include "SimulationEnvironmental.h"
#include "SimulationEventContext.h"
#include "StrainIdentity.h"
#include "NodeEnvironmental.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "PropertyReportEnvironmental" )

static const std::string _report_name = "PropertyReportEnvironmental.json";

// nasty copy-paste from ReportEnvironmental.
static const std::string _num_enviro_infections_label    = "New Infections By Route (ENVIRONMENT)";
static const std::string _num_contact_infections_label   = "New Infections By Route (CONTACT)";

namespace Kernel
{

GET_SCHEMA_STATIC_WRAPPER_IMPL( PropertyReportEnvironmental, PropertyReportEnvironmental )

        /////////////////////////
// Initialization methods
/////////////////////////
IReport*
PropertyReportEnvironmental::CreateReport()
{
    return _new_ PropertyReportEnvironmental();
}

PropertyReportEnvironmental::PropertyReportEnvironmental()
    : PropertyReport( _report_name )
{
}

PropertyReportEnvironmental::PropertyReportEnvironmental( const std::string& _report_name )
    : PropertyReport( _report_name )
{
}

void
PropertyReportEnvironmental::LogIndividualData(
    Kernel::IIndividualHuman* individual
)
{
    PropertyReport::LogIndividualData( individual );

    // Try an optimized solution that constructs a reporting bucket string based entirely
    // on the properties of the individual. But we need some rules. Let's start with simple
    // alphabetical ordering of category names
    std::string reportingBucket = individual->GetPropertyReportString();

    float mc_weight = (float)individual->GetMonteCarloWeight();
    const Kernel::IndividualHumanEnvironmental* individual_ty = static_cast<const Kernel::IndividualHumanEnvironmental*>(individual);

    if( individual->IsInfected() )
    {
        NewInfectionState::_enum nis = individual->GetNewInfectionState(); 
        if( nis == NewInfectionState::NewAndDetected ||
            nis == NewInfectionState::NewInfection )
        {
            auto inf = individual->GetInfections().back();
            StrainIdentity si;
            inf->GetInfectiousStrainID( &si );
            if( si.GetGeneticID() == 0 )
            {
                new_enviro_infections[ reportingBucket ] += mc_weight;
            }
            else if( si.GetGeneticID() == 1 )
            {
                new_contact_infections[ reportingBucket ] += mc_weight;
            }
        }
    }
}

void
PropertyReportEnvironmental::LogNodeData( INodeContext* pNC )
{
    PropertyReport::LogNodeData(pNC);

    LOG_DEBUG( "LogNodeData in PropertyReportEnvironmental\n" );
    for (const auto& entry : permutationsSet)
    {
        std::string reportingBucket = PropertiesToString( entry );

        //Accumulate("Active Environmental Infections:" + reportingBucket, active_infections[reportingBucket] );
        //active_infections[reportingBucket] = 0.0f;
        Accumulate( _num_enviro_infections_label + ":" + reportingBucket, new_enviro_infections[ reportingBucket ] );
        Accumulate( _num_contact_infections_label + ":" + reportingBucket, new_contact_infections[ reportingBucket ] );
        new_enviro_infections[ reportingBucket ] = 0;
        new_contact_infections[ reportingBucket ] = 0;
    }

    {
        // Grovel through the demographics to see which properties are being used for HINT.

        if ( IPFactory::GetInstance() && IPFactory::GetInstance()->HasIPs() )
        {
            for ( auto property : IPFactory::GetInstance()->GetIPList() )
            {
                auto nodeId = pNC->GetExternalID();
                auto hint = property->GetIntraNodeTransmission( nodeId );
                auto matrix = hint.GetMatrix();

                std::string routeName;

                if ( matrix.size() > 0 )
                {
                    routeName = hint.GetRouteName();
                    reportContagionForRoute( routeName, property, pNC );
                }
                else if ( hint.GetRouteToMatrixMap().size() > 0 )
                {
                    for (auto entry : hint.GetRouteToMatrixMap())
                    {
                        routeName = entry.first;
                        reportContagionForRoute( routeName, property, pNC );
                    }
                }
                else //HINT is enabled, but no transmission matrix is detected
                {
                    LOG_INFO_F( "IndividualProperty '%s' isn't part of HINT configuration.\n", property->GetKeyAsString().c_str() );
                }
            }
        }
        else
        {
            LOG_INFO( "PropertyReportEnvironmental didn't find any IndividualProperties.\n" );
        }
    }
}

void PropertyReportEnvironmental::reportContagionForRoute( const std::string& route, IndividualProperty* property, INodeContext* pNC )
{
    if ( (route != CONTACT) && (route != ENVIRONMENTAL) )
    {
        LOG_WARN_F( "Unknown route '%s' in IndividualProperties for node %d.\n", route.c_str(), pNC->GetExternalID() );
        return;
    }

    std::string prefix = (route == CONTACT) ? "Contagion (Contact):" : "Contagion (Environment):";
    for (auto& value : property->GetValues<IPKeyValueContainer>())
    {
        const string& label = value.ToString();
        auto contagion = pNC->GetContagionByRouteAndProperty( route, value );
        Accumulate( (prefix + label).c_str(), contagion );
    }
}

/*
void
PropertyReportEnvironmental::postProcessAccumulatedData()
{
    LOG_DEBUG( "postProcessAccumulatedData in PropertyReportEnvironmental\n" );
    PropertyReport::postProcessAccumulatedData();
}
*/
}

