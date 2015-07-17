/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "HIVSimpleDiagnostic.h"

namespace Kernel
{
    struct IIndividualHumanHIV;
    struct IHIVMedicalHistory;

    class IDMAPI HIVARTStagingAbstract : public HIVSimpleDiagnostic
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public: 
        HIVARTStagingAbstract();
        HIVARTStagingAbstract( const HIVARTStagingAbstract& );

        virtual bool Configure( const Configuration * inputJson );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );

    protected:
        virtual bool positiveTestResult();
        virtual void onNegativeTestResult();
        virtual void positiveTestDistribute();

        virtual bool positiveTestResult( IIndividualHumanHIV *pHIV, 
                                         float year, 
                                         float CD4count, 
                                         bool hasActiveTB, 
                                         bool isPregnant ) = 0;

        virtual void UpdateMedicalHistory( IHIVMedicalHistory *pMedHistory, bool isPositiveTestResult );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string ip_tb_key ;
        std::string ip_tb_value_expected ;
#pragma warning( pop )

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, HIVARTStagingAbstract &obj, const unsigned int v);
#endif
    };
}
