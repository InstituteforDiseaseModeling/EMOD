/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "stdafx.h"
#include "EventTrigger.h"
#include "SimulationConfig.h"  // for checking that event strings are valid

namespace Kernel
{
    EventTrigger::EventTrigger()
    : ConstrainedString( JsonConfigurable::default_string )
    {
        constraints = "<configuration>:Listed_Events.*";
    }

    EventTrigger::EventTrigger( std::string &init_str )
    : ConstrainedString( init_str )
    {
        constraints = "<configuration>:Listed_Events.*";
    }

    EventTrigger::EventTrigger( const char *init_str )
    : ConstrainedString( init_str )
    {
        constraints = "<configuration>:Listed_Events.*";
    }

    const jsonConfigurable::ConstrainedString& EventTrigger::operator=( const std::string& new_value )
    {
        constraint_param = &GET_CONFIGURABLE(SimulationConfig)->listed_events;
        return jsonConfigurable::ConstrainedString::operator=( new_value );
    }

    bool EventTrigger::IsUninitialized() const
    {
        return *this == JsonConfigurable::default_string;
    }

    void EventTrigger::serialize( Kernel::IArchive& ar, EventTrigger& obj )
    {
        ar & (jsonConfigurable::ConstrainedString&)(obj);
    }
}