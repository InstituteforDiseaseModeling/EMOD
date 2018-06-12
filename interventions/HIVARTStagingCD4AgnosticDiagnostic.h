/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "InterpolatedValueMap.h"
#include "HIVARTStagingAbstract.h"

namespace Kernel
{
    class IDMAPI HIVARTStagingCD4AgnosticDiagnostic : public HIVARTStagingAbstract
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVARTStagingCD4AgnosticDiagnostic, IDistributableIntervention)

    public: 
        HIVARTStagingCD4AgnosticDiagnostic();
        HIVARTStagingCD4AgnosticDiagnostic( const HIVARTStagingCD4AgnosticDiagnostic& );

        virtual bool Configure( const Configuration * inputJson ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;

    protected:
        virtual bool positiveTestResult( IIndividualHumanHIV *pHIV, 
                                         float year, 
                                         float CD4count, 
                                         bool hasActiveTB, 
                                         bool isPregnant ) override;

        virtual bool TestAdult( float WHO_Stage, float year, bool hasActiveTB, bool isPregnant );
        virtual bool TestChild( float WHO_Stage, float year, bool hasActiveTB, float ageDays   );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        float adultAge;

        InterpolatedValueMap adultByWHOStage,  childByWHOStage;
        InterpolatedValueMap adultByTB,        childByTB;
        InterpolatedValueMap adultByPregnant;
        InterpolatedValueMap childTreatUnderAgeThreshold;

        DECLARE_SERIALIZABLE(HIVARTStagingCD4AgnosticDiagnostic);
#pragma warning( pop )
    };
}
