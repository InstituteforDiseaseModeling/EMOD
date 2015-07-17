/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "SimulationEnums.h"
#include "Interventions.h"
#include "InterventionFactory.h"

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

    protected:
        virtual bool PreConfigure( const Configuration* config );
        virtual void DistributionConfigure( const Configuration* config );
        virtual void InterventionConfigure( const Configuration* config );
        virtual void InterventionValidate();
        virtual void DelayValidate();

        virtual void CalculateDelay();

        IIndividualHumanContext *parent;
        float remaining_delay_days;
        float coverage;

        DistributionFunction::Enum delay_distribution;
        float delay_period;
        float delay_period_min;
        float delay_period_max;
        float delay_period_mean;
        float delay_period_std_dev;

        IndividualInterventionConfig actual_intervention_config;

    private:
        // Serialization
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        template<class Archive>
        friend void serialize(Archive &ar, DelayedIntervention &obj, const unsigned int v);
#endif
    };
}
