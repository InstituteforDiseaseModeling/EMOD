/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "HIVSimpleDiagnostic.h"
#include "IHIVInterventionsContainer.h"

namespace Kernel
{
    struct IHIVMedicalHistory ;

    class HIVRapidHIVDiagnostic : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVRapidHIVDiagnostic, IDistributableIntervention)

    public: 
        HIVRapidHIVDiagnostic();
        HIVRapidHIVDiagnostic( const HIVRapidHIVDiagnostic& );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

    protected:
        virtual void onNegativeTestResult();
        virtual void positiveTestDistribute();
        virtual void onReceivedResult( IHIVMedicalHistory* pMedHistory, bool resultIsHivPositive );

        float m_ProbReceivedResults ;
    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, HIVRapidHIVDiagnostic &obj, const unsigned int v);
#endif
    };
}
