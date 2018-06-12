/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Interventions.h"
#include "InterventionFactory.h"

namespace Kernel
{
    class MultiNodeInterventionDistributor : public BaseNodeIntervention
    {
        DECLARE_FACTORY_REGISTERED( InterventionFactory, MultiNodeInterventionDistributor, INodeDistributableIntervention )

    public:
        MultiNodeInterventionDistributor();
        virtual ~MultiNodeInterventionDistributor();

        virtual bool Configure( const Configuration* config ) override;

        // IDistributableIntervention
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual void Update( float dt ) override;
        virtual bool Distribute( INodeEventContext *context, IEventCoordinator2* pEC = nullptr ) override;

    protected:
        NodeInterventionConfig intervention_list;
    };
}
