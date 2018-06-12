/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "VectorInterventionsContainer.h"
#include "MalariaContexts.h"
#include "MalariaInterventionsContainerContexts.h"

namespace Kernel
{
    class MalariaInterventionsContainer :
        public VectorInterventionsContainer,
        public IMalariaDrugEffects, // Getters
        public IMalariaDrugEffectsApply
    {
    public:
        // TODO - WHY IS THIS NECESSARY? Making compiler happy (but not me). Make go away soon.
        virtual int32_t AddRef() override;
        virtual int32_t Release() override;

        MalariaInterventionsContainer();
        virtual ~MalariaInterventionsContainer();

        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // IMalariaDrugEffectsApply
        virtual void AddDrugEffects( IMalariaDrugEffects* pDrugEffects ) override;
        virtual void RemoveDrugEffects( IMalariaDrugEffects* pDrugEffects ) override;

        //IMalariaDrugEffects
        virtual float get_drug_IRBC_killrate( const IStrainIdentity& rStrain ) override;
        virtual float get_drug_hepatocyte(    const IStrainIdentity& rStrain ) override;
        virtual float get_drug_gametocyte02(  const IStrainIdentity& rStrain ) override;
        virtual float get_drug_gametocyte34(  const IStrainIdentity& rStrain ) override;
        virtual float get_drug_gametocyteM(   const IStrainIdentity& rStrain ) override;

        virtual void InfectiousLoopUpdate(float dt) override; // update interventions in the infection loop

    protected:
        std::vector<IMalariaDrugEffects*> m_DrugEffectsCollection;

        DECLARE_SERIALIZABLE(MalariaInterventionsContainer);
    };
}
