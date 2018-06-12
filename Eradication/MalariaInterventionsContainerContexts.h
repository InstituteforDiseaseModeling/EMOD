/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ISupports.h"
#include "IStrainIdentity.h"

namespace Kernel
{
    struct IMalariaDrugEffects : public ISupports
    {
        virtual float get_drug_IRBC_killrate( const IStrainIdentity& rStrain ) = 0;
        virtual float get_drug_hepatocyte(    const IStrainIdentity& rStrain ) = 0;
        virtual float get_drug_gametocyte02(  const IStrainIdentity& rStrain ) = 0;
        virtual float get_drug_gametocyte34(  const IStrainIdentity& rStrain ) = 0;
        virtual float get_drug_gametocyteM(   const IStrainIdentity& rStrain ) = 0;
        virtual ~IMalariaDrugEffects()
        {
        }
    };

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! BEWARE:  This pattern of adding pointers to objects is not one that should be
    // !!! copied lightly.  It is better when the effects of the intervention go into something
    // !!! disconnected from the intervention like a primative value (i.e. integer, float)
    // !!! In this case, each infection can be effected differently by each drug, hence,
    // !!! we need to be able to get different information depending on the infection.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    struct IMalariaDrugEffectsApply : public ISupports
    {
        virtual void AddDrugEffects(    IMalariaDrugEffects* pDrugEffects ) = 0;
        virtual void RemoveDrugEffects( IMalariaDrugEffects* pDrugEffects ) = 0;
    };
}
