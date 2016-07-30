/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "MathFunctions.h"
#include "Interventions.h"
#include "InterventionFactory.h"
#include "DurationDistribution.h"
#include "Timers.h"

namespace Kernel
{
    class DelayedIntervention: public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, DelayedIntervention, IDistributableIntervention)

    public:
        DelayedIntervention();
        DelayedIntervention( const DelayedIntervention& );
        virtual ~DelayedIntervention();

        virtual bool Configure( const Configuration* config );
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );
        virtual void SetContextTo( IIndividualHumanContext *context );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void Update(float dt);

        virtual void Callback( float dt );

    protected:
        virtual void PreConfigure( const Configuration* config );
        virtual void DistributionConfigure( const Configuration* config );
        virtual void InterventionConfigure( const Configuration* config );
        virtual void InterventionValidate();
        virtual void DelayValidate();

        virtual void CalculateDelay();

        IIndividualHumanContext *parent;
        CountdownTimer remaining_delay_days;
        float coverage;

        DurationDistribution delay_distribution;

        IndividualInterventionConfig actual_intervention_config;

        DECLARE_SERIALIZABLE(DelayedIntervention);
    };
}
