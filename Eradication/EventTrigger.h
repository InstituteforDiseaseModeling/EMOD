/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"

namespace Kernel
{
    // An EventTrigger is string prepresenting an event that occurs to an individual.
    // EventTriggers are typically user defined events versus those defined in the
    // enum IndividualEventTriggerType.
    class EventTrigger : public jsonConfigurable::ConstrainedString
    {
    public:
        EventTrigger();
        EventTrigger( std::string &init_str );
        EventTrigger( const char *init_str );
        virtual const jsonConfigurable::ConstrainedString& operator=( const std::string& new_value ) override;
        bool IsUninitialized() const;
        static void serialize( Kernel::IArchive& ar, EventTrigger& obj );
    };
}