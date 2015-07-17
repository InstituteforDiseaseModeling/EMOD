/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

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
        virtual bool Configure( const Configuration* pConfig );
        virtual ~DiagnosticTreatNeg();// { }

        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );
        virtual void Update(float dt);

        virtual bool positiveTestResult();

        virtual void onNegativeTestResult();
        virtual void negativeTestDistribute();

        virtual void onPatientDefault();

        virtual float getTreatmentFractionNegative() const;

    protected:
        IndividualInterventionConfig negative_diagnosis_config;
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        ConstrainedString negative_diagnosis_event;

        IndividualInterventionConfig defaulters_config;
        ConstrainedString defaulters_event;
#pragma warning( pop )

    private:
        bool m_gets_positive_test_intervention;


#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        // Serialization
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, DiagnosticTreatNeg &obj, const unsigned int v);
#endif
    };
}
