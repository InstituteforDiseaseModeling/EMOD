
#pragma once

#include "IRelationship.h"
#include "MathFunctions.h"
#include "Sigmoid.h"

namespace Kernel
{
    struct IRelationshipParameters;

    // this container becomes a help implementation member of the IndividualHumanSTI class 
    // it needs to implement consumer interfaces for all the relevant intervention types

    // The StartNewRelationship intervention needs to create the relationship
    // at about the place in the code as the PFA.  This will ensure that all
    // relationships are processed in the same order.  This was discovered
    // because if you created the relationship when the intervention was updated,
    // it can expire before the couple has a chance to consummate.
    struct INonPfaRelationshipStarter : ISupports
    {
        virtual void StartNonPfaRelationship() = 0;
    };

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

    struct ICoitalActRiskData : ISupports
    {
        virtual void UpdateCoitalActRiskFactors( float acqMod, float tranMod ) = 0;
        virtual float GetCoitalActRiskAcquisitionFactor() const = 0;
        virtual float GetCoitalActRiskTransmissionFactor() const = 0;
    };

    struct ISTIInterventionsContainer : public ISupports
    {
        // These two methods are added for performance.  We could use QueryInterface
        // before putting the files in the container or when we are trying to get them out.
        // However, this lets us only do this for this intervention and not everything.
        virtual void AddNonPfaRelationshipStarter( INonPfaRelationshipStarter* pSNR ) = 0;
        virtual void StartNonPfaRelationships() = 0;
    };

}
