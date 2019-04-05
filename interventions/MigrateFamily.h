/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "Configuration.h"
#include "Configure.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "IDistribution.h"
#include "INodeContext.h"

namespace Kernel
{
    class MigrateFamily :  public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, MigrateFamily, INodeDistributableIntervention)

    public: 
        MigrateFamily();
        MigrateFamily( const MigrateFamily& master );
        virtual ~MigrateFamily();
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual bool Distribute( INodeEventContext *pNodeEventContext, IEventCoordinator2 *pEC ) override;
        virtual void Update(float dt) override;

    protected:

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        ExternalNodeId_t destination_external_node_id ;
        IDistribution* duration_before_leaving;
        IDistribution* duration_at_node;
        bool is_moving;

        // TODO - can't do until BaseNodeIntervention is serializable
        //DECLARE_SERIALIZABLE(MigrateFamily);
#pragma warning( pop )
    };
}
