/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "SimulationEnums.h"
#include "DelayedIntervention.h"
#include "Configure.h"
#include "InterpolatedValueMap.h"
#include "IHIVCascadeStateIntervention.h"
#include <string>

namespace Kernel
{
    class HIVDelayedIntervention: public DelayedIntervention, public IHIVCascadeStateIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVDelayedIntervention, IDistributableIntervention)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    
    public: 
        HIVDelayedIntervention();
        HIVDelayedIntervention( const HIVDelayedIntervention& );

        virtual bool Configure( const Configuration* config );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        // todo: lift to HIVIntervention::Update
        virtual void Update(float dt);

        // IHIVCascadeStateIntervention
        virtual const std::string& GetCascadeState();
        virtual const JsonConfigurable::tDynamicStringSet& GetAbortStates();

    protected:
        virtual void CalculateDelay();

        // todo: lift to HIVIntervention -- repeated in HIVSimpleDiagnostic
        virtual bool UpdateCascade();
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO);

        void AbortValidate();
        bool AbortDueToCurrentCascadeState();
        bool qualifiesToGetIntervention( IIndividualHumanContext* pIndivid );

        InterpolatedValueMap year2DelayMap;
        JsonConfigurable::tDynamicStringSet abortStates;
        std::string cascadeState;
        float days_remaining;
        bool firstUpdate;

        ConstrainedString broadcast_event;
        ConstrainedString broadcast_on_expiration_event;

    private:
        // Serialization
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        template<class Archive>
        friend void serialize(Archive &ar, HIVDelayedIntervention &obj, const unsigned int v);
#endif
    };
}
