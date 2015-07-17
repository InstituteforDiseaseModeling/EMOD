/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "InterventionFactory.h"

namespace Kernel
{
    class MultiInterventionDistributor : public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, MultiInterventionDistributor, IDistributableIntervention)
    
    public: 
        MultiInterventionDistributor();
        virtual ~MultiInterventionDistributor();

        bool Configure( const Configuration* config );

        // IDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );

    protected:
        virtual void Expire();

        IIndividualHumanContext *parent;
		IndividualInterventionConfig intervention_list; // TBD

    private:

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        template<class Archive>
        friend void serialize(Archive &ar, MultiInterventionDistributor &obj, const unsigned int v);
#endif
    };
}
