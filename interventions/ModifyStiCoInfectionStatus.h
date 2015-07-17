/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>

#include "Interventions.h"
#include "InterventionFactory.h"

namespace Kernel
{
    class ModifyStiCoInfectionStatus : public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(ModifyStiCoInfectionStatus)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ModifyStiCoInfectionStatus, IDistributableIntervention)
        DECLARE_QUERY_INTERFACE()

    public:
        ModifyStiCoInfectionStatus();
        virtual ~ModifyStiCoInfectionStatus() { }

        // INodeDistributableIntervention
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual void SetContextTo(IIndividualHumanContext *context) { /* not needed for this intervention */ }
        virtual void Update(float dt);

    protected:
        bool set_flag_to;

    private:

#if USE_BOOST_SERIALIZATION
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, ModifyStiCoInfectionStatus &ob, const unsigned int v);
#endif
    };
}
