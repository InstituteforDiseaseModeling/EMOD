/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "MaleCircumcision.h" // for ICircumcision interface
#include "IRelationship.h"

namespace Kernel
{
    // this container becomes a help implementation member of the IndividualHumanSTI class 
    // it needs to implement consumer interfaces for all the relevant intervention types

    typedef struct {
        float early;
        float late;
        float midyear;
        float rate;
    } SigmoidConfig;

    struct ISTIBarrierConsumer : public ISupports
    {
        virtual void UpdateSTIBarrierProbabilitiesByType( RelationshipType::Enum rel_type, SigmoidConfig config_overrides ) = 0;
        virtual SigmoidConfig GetSTIBarrierProbabilitiesByRelType( RelationshipType::Enum rel_type ) const = 0;
    };

    struct ISTICircumcisionConsumer : public ISupports
    {
        virtual bool IsCircumcised( void ) const = 0;
        virtual bool ApplyCircumcision( ICircumcision *) = 0;
    };

    class ISTICoInfectionStatusChangeApply : public ISupports
    {
        public:
        virtual void SpreadStiCoInfection() = 0;
        virtual void CureStiCoInfection() = 0;
    };

    struct ISTIInterventionsContainer : public ISupports
    {
        // No methods currently
    };

}
