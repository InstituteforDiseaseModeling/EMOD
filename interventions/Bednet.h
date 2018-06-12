/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "IWaningEffect.h"
#include "EventTrigger.h"

namespace Kernel
{

    struct IBednetConsumer;

    class AbstractBednet : public BaseIntervention
    {
    public:
        AbstractBednet();
        AbstractBednet( const AbstractBednet& );
        virtual ~AbstractBednet();

        virtual bool Configure( const Configuration * config ) override;

        // IDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO ) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual void Update( float dt ) override;

    protected:
        virtual bool ConfigureBlockingAndKilling( const Configuration* config );     // calls BaseIntervention::Configure()
        virtual bool ConfigureUsage(  const Configuration* config ) = 0;             // should call JsonConfigurable::Configure()
        virtual bool ConfigureEvents( const Configuration* config ) { return true; } // should call JsonConfigurable::Configure()

        virtual void UpdateBlockingAndKilling( float dt );
        virtual void UpdateUsage( float dt ) = 0;
        virtual bool IsUsingBednet() const = 0;
        virtual bool CheckExpiration( float dt ) = 0;

        virtual void UseBednet();

        virtual float GetEffectKilling() const;
        virtual float GetEffectBlocking() const;
        virtual float GetEffectUsage() const = 0;

        void BroadcastEvent( const EventTrigger& trigger ) const;

        IWaningEffect* m_pEffectKilling;
        IWaningEffect* m_pEffectBlocking;
        IBednetConsumer *m_pConsumer;

        static void serialize( IArchive& ar, AbstractBednet* obj );
    };

    class SimpleBednet : public AbstractBednet
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleBednet, IDistributableIntervention)

    public:
        SimpleBednet();
        SimpleBednet( const SimpleBednet& );
        virtual ~SimpleBednet();

        // IDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo( IIndividualHumanContext *context ) override;

    protected:
        virtual bool ConfigureUsage( const Configuration* config ) override;
        virtual void UpdateUsage( float dt ) override;
        virtual bool IsUsingBednet() const override;
        virtual bool CheckExpiration( float dt ) override;

        virtual float GetEffectKilling() const override;
        virtual float GetEffectBlocking() const override;
        virtual float GetEffectUsage() const override;

        IWaningEffect* m_pEffectUsage;

        DECLARE_SERIALIZABLE(SimpleBednet);
    };
}
