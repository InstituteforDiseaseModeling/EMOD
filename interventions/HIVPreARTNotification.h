/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "FactorySupport.h"

namespace Kernel
{

    class HIVPreARTNotification : public BaseIntervention
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVPreARTNotification, IDistributableIntervention)

        HIVPreARTNotification();
        virtual ~HIVPreARTNotification() { }

        virtual bool Configure( const Configuration * config );

        // IDistributableIntervention
        virtual void SetContextTo(IIndividualHumanContext *context) { }
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void Update(float dt);

    protected:
        bool startingPreART;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<typename Archive>
        friend void serialize( Archive &ar, HIVPreARTNotification& obj, unsigned int version );
#endif
    };
}
