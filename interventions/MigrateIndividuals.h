/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "Configuration.h"
#include "Configure.h"
#include "Contexts.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "DurationDistribution.h"
#include "INodeContext.h"

namespace Kernel
{
    class IDMAPI MigrateIndividuals :  public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, MigrateIndividuals, IDistributableIntervention)

    public: 
        MigrateIndividuals();
        MigrateIndividuals( const MigrateIndividuals& master );
        virtual ~MigrateIndividuals() {  }
        virtual bool Configure( const Configuration* pConfig ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

    protected:

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        ExternalNodeId_t destination_external_node_id;
        DurationDistribution duration_before_leaving;
        DurationDistribution duration_at_node;
        bool is_moving;

        DECLARE_SERIALIZABLE(MigrateIndividuals);
#pragma warning( pop )
    };
}
