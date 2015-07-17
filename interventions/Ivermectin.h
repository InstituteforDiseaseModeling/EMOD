/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <map>

#include "Interventions.h"
#include "InterventionFactory.h"
#include "Configuration.h"
#include "WaningEffect.h"
#include "Configure.h"

namespace Kernel
{
    struct IVectorInterventionEffectsSetter;

    class Ivermectin : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, Ivermectin, IDistributableIntervention)

    public:
        bool Configure( const Configuration * config );
        Ivermectin();
        Ivermectin( const Ivermectin& );
        virtual ~Ivermectin() { }

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver  * const pCCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

    protected:
        WaningConfig killing_config;
        IWaningEffect* killing_effect;
        IVectorInterventionEffectsSetter *ivies; // aka individual vector interventions container

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, Ivermectin& ivm, const unsigned int v);
#endif
    };
}
