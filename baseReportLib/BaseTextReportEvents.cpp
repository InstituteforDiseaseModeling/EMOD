/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "BaseTextReportEvents.h"
#include "NodeEventContext.h"
#include "EventTriggerCoordinator.h"

SETUP_LOGGING( "BaseTextReportEvents" )

// moved after SETUP_LOGGING to make Linux Debug happy with LOG_DEBUG statements
#include "BaseTextReportEventsTemplate.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- BaseTextReportEvents
    // ------------------------------------------------------------------------

    BaseTextReportEvents::BaseTextReportEvents( const std::string& rReportName )
        : BaseTextReportEventsTemplate( rReportName )
    {
    }

    BaseTextReportEvents::~BaseTextReportEvents()
    {
    }

    void BaseTextReportEvents::UpdateEventRegistration( float currentTime,
                                                        float dt,
                                                        std::vector<INodeEventContext*>& rNodeEventContextList,
                                                        ISimulationEventContext* pSimEventContext )
    {
        if( !is_registered )
        {
            for( auto pNEC : rNodeEventContextList )
            {
                release_assert( pNEC );
                IIndividualEventBroadcaster* broadcaster = pNEC->GetIndividualEventBroadcaster();
                UpdateRegistration( broadcaster, true );
                broadcaster_list.push_back( broadcaster );
            }
            is_registered = true;
        }
    }


}
