/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IRelationship.h"
#include "MathFunctions.h"
#include "Sigmoid.h"

namespace Kernel
{
    struct IRelationshipParameters;

    // this container becomes a help implementation member of the IndividualHumanSTI class 
    // it needs to implement consumer interfaces for all the relevant intervention types

    struct ISTIBarrierConsumer : public ISupports
    {
        virtual void UpdateSTIBarrierProbabilitiesByType( RelationshipType::Enum rel_type, const Sigmoid& config_overrides ) = 0;
        virtual const Sigmoid& GetSTIBarrierProbabilitiesByRelType( const IRelationshipParameters* pRelParams ) const = 0;
    };

    struct ISTICircumcisionConsumer : public ISupports
    {
        virtual bool IsCircumcised( void ) const = 0;
        virtual float GetCircumcisedReducedAcquire() const = 0;
        virtual void ApplyCircumcision( float reduceAcquire ) = 0;
    };

    struct ISTICoInfectionStatusChangeApply : public ISupports
    {
        virtual void SpreadStiCoInfection() = 0;
        virtual void CureStiCoInfection() = 0;
    };

    struct ISTIInterventionsContainer : public ISupports
    {
        // No methods currently
    };

}
