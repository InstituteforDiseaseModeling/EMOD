/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <string>
#include "BaseTextReportEvents.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "BaseTextReportEvents" )

namespace Kernel
{
    BaseTextReportEvents::BaseTextReportEvents( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , eventTriggerList()
        , ntic_list()
        , is_registered(false)
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    BaseTextReportEvents::~BaseTextReportEvents()
    {
    }

    void BaseTextReportEvents::UpdateEventRegistration( float currentTime, 
                                                        float dt, 
                                                        std::vector<INodeEventContext*>& rNodeEventContextList )
    {
        if( !is_registered )
        {
            for( auto pNEC : rNodeEventContextList )
            {
                release_assert( pNEC );
                INodeTriggeredInterventionConsumer* pNTIC = nullptr;
                if( pNEC->QueryInterface( GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC ) != s_OK )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNEC", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
                }
                release_assert( pNTIC );

                UpdateRegistration( pNTIC, true );
                ntic_list.push_back( pNTIC ) ;
            }
            is_registered = true ;
        }
    }


    void BaseTextReportEvents::Reduce()
    {
        BaseTextReport::Reduce();
        if( is_registered )
        {
            UnregisterAllNodes();
        }
    }

    void BaseTextReportEvents::UpdateRegistration( INodeTriggeredInterventionConsumer* pNTIC,
                                                   bool registering )
    {

        for( auto trigger : eventTriggerList )
        {
            if( registering )
            {
                LOG_DEBUG_F( "BaseTextReportEvents is registering to listen to event %s\n", trigger.c_str() );
                pNTIC->RegisterNodeEventObserver( this, trigger );
            }
            else
            {
                pNTIC->UnregisterNodeEventObserver( this, trigger );
            }
        }
    }

    void BaseTextReportEvents::UnregisterAllNodes()
    {
        for( auto pNTIC : ntic_list )
        {
            UpdateRegistration( pNTIC, false );
            pNTIC->Release();
        }
        ntic_list.clear();
    }

}
