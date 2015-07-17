/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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

        virtual bool Configure( const Configuration* config );
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        // lift to HIVIntervention::Update
        virtual void Update(float dt);

    protected:
        virtual void DelayValidate();
        virtual void CalculateDelay();

        int max_entries;
        std::string muxer_name;

    private:
        // Serialization
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        template<class Archive>
        friend void serialize(Archive &ar, HIVMuxer &obj, const unsigned int v);
#endif
    };
}
