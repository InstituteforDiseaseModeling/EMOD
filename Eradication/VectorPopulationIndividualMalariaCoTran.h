
#pragma once

#include "VectorPopulationIndividual.h"

namespace Kernel
{
    struct INodeMalariaCoTransmission;

    class VectorPopulationIndividualMalariaCoTran : public VectorPopulationIndividual
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        static VectorPopulationIndividualMalariaCoTran *CreatePopulation( INodeContext *context,
                                                                          int speciesIndex,
                                                                          uint32_t _adult,
                                                                          uint32_t mosquito_weight );
        virtual ~VectorPopulationIndividualMalariaCoTran() override;

        virtual void SetContextTo(INodeContext *context) override;

    protected:
        VectorPopulationIndividualMalariaCoTran(uint32_t mosquito_weight=1); // default needed for serialization

        virtual void queueIncrementNumInfs( IVectorCohort* cohort ) override;
        virtual void AcquireNewInfection( uint32_t vectorID,
                                          IVectorCohortIndividual* pVCI,
                                          const StrainIdentity& rStrain,
                                          bool isIndoors );

        INodeMalariaCoTransmission *m_pNodeCoTran;

        DECLARE_SERIALIZABLE(VectorPopulationIndividualMalariaCoTran);
    };
}
