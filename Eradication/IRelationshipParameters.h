/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IRelationship.h"
#include "Sigmoid.h"

namespace Kernel 
{
    struct IDMAPI IRelationshipParameters
    {
        virtual RelationshipType::Enum GetType() const = 0;

        virtual float   GetCoitalActRate()                const = 0;
        virtual float   GetDurationWeibullHeterogeneity() const = 0;
        virtual float   GetDurationWeibullScale()         const = 0;
        virtual const Sigmoid& GetCondomUsage()           const = 0;

        // the size of these vectors should be equal with a one-to-one corespondence between the action and the probability.
        virtual const std::vector<RelationshipMigrationAction::Enum>& GetMigrationActions() const = 0;
        virtual const std::vector<float>& GetMigrationActionsCDF() const = 0;
    };
}