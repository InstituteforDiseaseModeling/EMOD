
#pragma once
#include <vector>
#include "ISupports.h"
#include "IRelationshipManager.h"
#include "ISociety.h"

using namespace std;

namespace Kernel
{
    class CoitalAct;
    struct IStrainIdentity;
    struct IDistributableIntervention;
    struct IIndividualHumanEventContext;
    class RANDOMBASE;

    // Add interventions/events to be distributed/broadcasted after all of the individuals have been updated.
    // The goal is to avoid situations where the order of the individuals being processed changes the outcome.
    struct IActionStager
    {
        virtual void StageIntervention( IIndividualHumanEventContext* pHuman, IDistributableIntervention* pIntervention ) = 0;
        virtual void StageEvent( IIndividualHumanEventContext* pHuman, const EventTrigger& rTrigger ) = 0;
    };

    struct INodeSTI : public ISupports
    {
    public:
        virtual /*const?*/ IRelationshipManager* GetRelationshipManager() /*const?*/ = 0;
        virtual ISociety* GetSociety() = 0;
        virtual IActionStager* GetActionStager() = 0;

        virtual void DepositFromIndividual( const IStrainIdentity& rStrain, const CoitalAct& rCoitalAct ) = 0;
        virtual RANDOMBASE* GetRng() = 0;
    };
}