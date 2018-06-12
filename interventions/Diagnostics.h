/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "Configuration.h"
#include "Configure.h"
#include "Contexts.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "Timers.h"
#include "Types.h"
#include "EventTrigger.h"

namespace Kernel
{
    class IDMAPI SimpleDiagnostic :  public BaseIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleDiagnostic, IDistributableIntervention)

    public:
        SimpleDiagnostic();
        SimpleDiagnostic( const SimpleDiagnostic& master );
        virtual ~SimpleDiagnostic() {  }
        bool Configure( const Configuration* pConfig ) override;
        void ConfigurePositiveEventOrConfig( const Configuration * inputJson );

        // IDistributingDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) override;
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void Update(float dt) override;

        virtual bool positiveTestResult();
        virtual void onNegativeTestResult();
        virtual void onPatientDefault();
        virtual void positiveTestDistribute();
        virtual bool applySensitivityAndSpecificity( bool infected ) const;
        virtual void Callback( float dt );

    protected:

        void broadcastEvent( const EventTrigger& event );
        virtual EventOrConfig::Enum getEventOrConfig( const Configuration* );
        void CheckPostiveEventConfig();

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        int   diagnostic_type;
        ProbabilityNumber base_specificity;
        ProbabilityNumber base_sensitivity;
        ProbabilityNumber treatment_fraction;
        CountdownTimer days_to_diagnosis; // can go negative if dt is > 1

        EventOrConfig::Enum use_event_or_config;
        IndividualInterventionConfig positive_diagnosis_config;
        EventTrigger positive_diagnosis_event;

        DECLARE_SERIALIZABLE(SimpleDiagnostic);
#pragma warning( pop )
    };
}
