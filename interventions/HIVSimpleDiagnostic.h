/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Diagnostics.h"
#include "IHIVCascadeStateIntervention.h"


namespace Kernel
{
    class IDMAPI HIVSimpleDiagnostic : public SimpleDiagnostic, public IHIVCascadeStateIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVSimpleDiagnostic, IDistributableIntervention)

    public:
        HIVSimpleDiagnostic();
        HIVSimpleDiagnostic( const HIVSimpleDiagnostic& );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void Update(float dt);
        virtual bool Configure(const Configuration* inputJson);
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );

        // SimpleDiagnostic
        virtual void onNegativeTestResult();

        // IHIVCascadeStateIntervention
        virtual const std::string& GetCascadeState();
        virtual const jsonConfigurable::tDynamicStringSet& GetAbortStates();

    protected:
        virtual void Callback( float dt );
        virtual bool qualifiesToGetIntervention( IIndividualHumanContext* pIndivid );
        virtual bool AbortDueToCurrentCascadeState();
        virtual bool UpdateCascade();
        virtual void ActOnResultsIfTime();
        virtual EventOrConfig::Enum getEventOrConfig( const Configuration* );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        jsonConfigurable::tDynamicStringSet abortStates;
        std::string cascadeState;
        bool firstUpdate;
        bool result_of_positive_test;
        float original_days_to_diagnosis;
        float absoluteDuration;
        EventTrigger negative_diagnosis_event;

        DECLARE_SERIALIZABLE(HIVSimpleDiagnostic);
#pragma warning( pop )
    };
}
