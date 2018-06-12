/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "HIVSimpleDiagnostic.h"
#include "Properties.h"

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

        virtual bool Configure( const Configuration * inputJson ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        virtual bool positiveTestResult() override;
        virtual void onNegativeTestResult() override;
        virtual void positiveTestDistribute() override;

        virtual bool positiveTestResult( IIndividualHumanHIV *pHIV, 
                                         float year, 
                                         float CD4count, 
                                         bool hasActiveTB, 
                                         bool isPregnant ) = 0;

        virtual void UpdateMedicalHistory( IHIVMedicalHistory *pMedHistory, bool isPositiveTestResult );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        IPKeyValue ip_tb_value_expected ;

        static void serialize( IArchive& ar, HIVARTStagingAbstract* obj );
#pragma warning( pop )
    };
}
