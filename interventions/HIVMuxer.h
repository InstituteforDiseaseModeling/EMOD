/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "SimulationEnums.h"
#include "HIVDelayedIntervention.h"
#include "Configure.h"
#include <string>

namespace Kernel
{
    class HIVMuxer: public HIVDelayedIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVMuxer, IDistributableIntervention)
    
    public: 
        HIVMuxer();
        HIVMuxer( const HIVMuxer & );

        virtual bool Configure( const Configuration* config ) override;
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

        virtual void Update(float dt) override;

    protected:
        virtual void DelayValidate() override;
        virtual void CalculateDelay() override;

        int max_entries;
        std::string muxer_name;
        bool firstUpdate;

        DECLARE_SERIALIZABLE(HIVMuxer);
    };
}
