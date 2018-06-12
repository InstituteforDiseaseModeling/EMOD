/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Diagnostics.h"

namespace Kernel
{
    class DiagnosticTreatNeg : public SimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, DiagnosticTreatNeg, IDistributableIntervention)

    public:
        DiagnosticTreatNeg();
        DiagnosticTreatNeg( const DiagnosticTreatNeg& master );
        virtual ~DiagnosticTreatNeg();// { }
        virtual bool Configure( const Configuration* pConfig ) override;

        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;
        virtual void Update(float dt) override;

        virtual bool positiveTestResult() override;

        void onDiagnosisComplete( float dt );
        virtual void onNegativeTestResult() override;
        virtual void negativeTestDistribute();

        virtual void onPatientDefault() override;

        virtual float getTreatmentFractionNegative() const;

    protected:
        IndividualInterventionConfig negative_diagnosis_config;
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        EventTrigger negative_diagnosis_event;

        IndividualInterventionConfig defaulters_config;
        EventTrigger defaulters_event;
#pragma warning( pop )

    private:
        bool m_gets_positive_test_intervention;

        DECLARE_SERIALIZABLE(DiagnosticTreatNeg);
    };
}
