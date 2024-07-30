
#pragma once

#include "AbstractDecision.h"
#include "Properties.h"

namespace Kernel
{
    struct IIndividualHumanHIV;
    struct IHIVMedicalHistory;

    class IDMAPI HIVARTStagingAbstract : public AbstractDecision
    {
    public:
        HIVARTStagingAbstract();
        HIVARTStagingAbstract( const HIVARTStagingAbstract& );

        virtual bool Configure( const Configuration * inputJson ) override;

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;

    protected:
        virtual void DistributeResultPositive() override;
        virtual void DistributeResultNegative() override;
        virtual bool MakeDecision( float dt ) override;

        virtual bool MakeDecision( IIndividualHumanHIV *pHIV, 
                                   float year, 
                                   float CD4count, 
                                   bool hasActiveTB, 
                                   bool isPregnant ) = 0;

        virtual void UpdateMedicalHistory( IHIVMedicalHistory *pMedHistory, bool isPositiveTestResult );

        IPKeyValue ip_tb_value_expected ;

        static void serialize( IArchive& ar, HIVARTStagingAbstract* obj );
    };
}
