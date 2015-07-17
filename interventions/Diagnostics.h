/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "Configuration.h"
#include "Configure.h"
#include "Contexts.h"
#include "HealthSeekingBehavior.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "Interventions.h"
#include "SimpleTypemapRegistration.h"

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
        bool Configure( const Configuration* pConfig );
        void ConfigurePositiveEventOrConfig( const Configuration * inputJson );

        // IDistributingDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(IIndividualHumanContext *context) { parent = context; } // for rng
        virtual void Update(float dt);

        virtual bool positiveTestResult();
        virtual void onNegativeTestResult();
        virtual void onPatientDefault();
        virtual void positiveTestDistribute();

    protected:

        void broadcastEvent(const std::string& event);
        virtual EventOrConfig::Enum getEventOrConfig( const Configuration* );

        IIndividualHumanContext *parent;
        int   diagnostic_type;
        float base_specificity;
        float base_sensitivity;
        float treatment_fraction;
        float days_to_diagnosis;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        IndividualInterventionConfig positive_diagnosis_config;
        ConstrainedString positive_diagnosis_event;
#pragma warning( pop )

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, SimpleDiagnostic &obj, const unsigned int v);
#endif
    };
}
