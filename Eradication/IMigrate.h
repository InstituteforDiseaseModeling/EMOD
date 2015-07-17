/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "ISupports.h"

namespace Kernel
{
    class Node;

    struct IMigrate : public ISupports
    {
        virtual void ImmigrateTo(Node* node) = 0;
        virtual void SetMigrationDestination(suids::suid destination) = 0;
        virtual const suids::suid & GetMigrationDestination() = 0;

        virtual ~IMigrate() {}
    };
}
