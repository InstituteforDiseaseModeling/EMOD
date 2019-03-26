/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "InterventionEnums.h"
#include "EventCoordinator.h"
#include "Configure.h"

namespace Kernel
{
    class StrainIdentity;
    struct IIndividualHumanContext;

    struct IOutbreakIndividual : public ISupports
    {
        virtual int GetAntigen() const = 0;
        virtual int GetGenome() const = 0;
        virtual ~IOutbreakIndividual() { }; // needed for cleanup via interface pointer
    };

    class OutbreakIndividual : public IOutbreakIndividual, public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(OutbreakIndividual)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, OutbreakIndividual, IDistributableIntervention)

    public:
        OutbreakIndividual();
        virtual ~OutbreakIndividual() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual void SetContextTo(IIndividualHumanContext *context) { /* not needed for this intervention */ }
        virtual void Update(float dt);

        // IOutbreakIndividual
        virtual int GetAntigen() const  { return antigen; }
        virtual int GetGenome() const  { return genome; }

        // other methods
        virtual void ConfigureAntigen( const Configuration * inputJson );
        virtual void ConfigureGenome( const Configuration * inputJson );

    protected:
        const StrainIdentity* GetNewStrainIdentity(INodeEventContext *context, IIndividualHumanContext* pIndiv);

        int antigen;
        int genome;
        bool ignoreImmunity;
        int incubation_period_override;
    };
}
