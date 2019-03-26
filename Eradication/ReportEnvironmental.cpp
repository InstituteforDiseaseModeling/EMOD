/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#if defined(ENABLE_ENVIRONMENTAL)

#include "ReportEnvironmental.h"
#include "Individual.h"
#include "StrainIdentity.h"
#include "INodeContext.h"

SETUP_LOGGING( "ReportEnvironmental" )

namespace Kernel {

    static const std::string _num_enviro_infections_label    = "New Infections By Route (ENVIRONMENT)";
    static const std::string _num_contact_infections_label   = "New Infections By Route (CONTACT)";

    ReportEnvironmental::ReportEnvironmental()
    {}

    void
    ReportEnvironmental::LogIndividualData( IIndividualHuman * individual )
    {
        Report::LogIndividualData( individual );

        if( individual->IsInfected() )
        {
            // Get infection incidence by route
            NewInfectionState::_enum nis = individual->GetNewInfectionState(); 
            LOG_VALID_F( "nis = %d\n", ( nis ) );
            auto mcw = individual->GetMonteCarloWeight();
            if ( nis == NewInfectionState::NewAndDetected ||
                 nis == NewInfectionState::NewInfection /*||
                 nis == NewInfectionState::NewlyDetected*/ )
            {
                auto inf = individual->GetInfections().back();
                StrainIdentity strain;
                inf->GetInfectiousStrainID( &strain );
                if( strain.GetGeneticID() == 0 )
                {
                    enviro_infections_counter += mcw;
                }
                else if( strain.GetGeneticID() == 1 )
                {
                    contact_infections_counter += mcw;
                }
            }
        }
    }

    void
    ReportEnvironmental::LogNodeData( INodeContext * pNC )
    {
        Report::LogNodeData( pNC );
       
        auto contagionPop = pNC->GetContagionByRoute();
        NonNegativeFloat contactContagionPop = contagionPop["contact"];
        NonNegativeFloat enviroContagionPop = contagionPop["environmental"];
        LOG_DEBUG_F( "Recording %f as 'Contact Contagion Population'.\n", float(contactContagionPop) );
        LOG_DEBUG_F( "Recording %f as 'Environmental  Contagion Population'.\n", float(enviroContagionPop) );
        Accumulate( "Contact Contagion Population", contactContagionPop  );
        Accumulate( "Environmental Contagion Population", enviroContagionPop );
    }

    void ReportEnvironmental::EndTimestep( float currentTime, float dt )
    {
        Accumulate( _num_enviro_infections_label, enviro_infections_counter );
        Accumulate( _num_contact_infections_label, contact_infections_counter );
        enviro_infections_counter = 0;
        contact_infections_counter = 0;
        Report::EndTimestep( currentTime, dt );
    }
}

#endif // ENABLE_ENVIRONMENTAL
