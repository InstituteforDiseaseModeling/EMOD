/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "ISTIInterventionsContainer.h"
#include "Interventions.h"
#include "InterventionsContainer.h"
#include "IRelationship.h"

namespace Kernel
{
    // this container becomes a help implementation member of the IndividualHumanSTI class 
    // it needs to implement consumer interfaces for all the relevant intervention types


    class STIInterventionsContainer
        : public InterventionsContainer
        , public ISTIInterventionsContainer
        , public ISTIBarrierConsumer
        , public ISTICircumcisionConsumer 
        , public ISTICoInfectionStatusChangeApply
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        STIInterventionsContainer();
        virtual ~STIInterventionsContainer();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // ISTIBarrierConsumer 
        virtual void UpdateSTIBarrierProbabilitiesByType( RelationshipType::Enum rel_type, const Sigmoid& config_overrides ) override;
        virtual const Sigmoid& GetSTIBarrierProbabilitiesByRelType( const IRelationshipParameters* pRelParams ) const override;

        // ISTICircumcisionConsumer 
        virtual bool IsCircumcised( void ) const override;
        virtual float GetCircumcisedReducedAcquire() const override;
        virtual void ApplyCircumcision( float reduceAcquire ) override;

        // IIndividualHumanInterventionsContext
        virtual void ChangeProperty( const char *property, const char* new_value) override;

        virtual float GetInterventionReducedAcquire() const override;
        virtual float GetInterventionReducedTransmit() const override;

        // ISTICoInfectionStatusChangeApply 
        virtual void SpreadStiCoInfection() override;
        virtual void CureStiCoInfection() override;

    protected:
        bool is_circumcised;
        float circumcision_reduced_require;

        std::map< RelationshipType::Enum, Sigmoid > STI_blocking_overrides;

        DECLARE_SERIALIZABLE(STIInterventionsContainer);
    };
}
