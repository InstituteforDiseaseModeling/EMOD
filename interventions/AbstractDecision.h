
#pragma once

#include "Interventions.h"
#include "InterventionFactory.h"
#include "EventTrigger.h"

namespace Kernel
{
    class AbstractDecision : public BaseIntervention
    {
    public:
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        AbstractDecision( const AbstractDecision& rMaster );
        virtual ~AbstractDecision();

        virtual bool Configure( const Configuration * ) override;

        // IDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;
        virtual void Update( float dt ) override;

    protected:
        AbstractDecision( bool hasNegativeResponse );

        virtual void ConfigureResponsePositive( const Configuration * inputJson );
        virtual void ConfigureResponseNegative( const Configuration * inputJson );
        virtual void CheckResponsePositive();
        virtual void CheckResponseNegative();
        virtual void DistributeResult( const EventTrigger& resultEvent );
        virtual void DistributeResultPositive();
        virtual void DistributeResultNegative();

        virtual bool MakeDecision( float dt ) = 0;

        bool m_HasNegativeResponse;
        EventTrigger m_EventPositive;
        EventTrigger m_EventNegative;

        static void serialize( IArchive& ar, AbstractDecision* obj );
    };
}
