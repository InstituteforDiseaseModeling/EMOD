/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "ISupports.h"

namespace Kernel
{
    // TODO - These are called consumers for historical reasons, might want to merge into IVectorInterventionsEffects
    struct IHousingModificationConsumer : public ISupports
    {
        virtual void ApplyHouseBlockingProbability( float prob ) = 0;
        virtual void UpdateProbabilityOfScreenKilling( float prob ) = 0;
    };

    struct IBednetConsumer : public ISupports
    {
        virtual void UpdateProbabilityOfBlocking( float prob ) = 0;
        virtual void UpdateProbabilityOfKilling( float prob ) = 0;
    };

    struct IIndividualRepellentConsumer : public ISupports
    {
        virtual void UpdateProbabilityOfIndRepBlocking( float prob ) = 0;
        virtual void UpdateProbabilityOfIndRepKilling( float prob ) = 0;
    };

    struct IVectorInterventionEffectsSetter : public ISupports
    {
        virtual void UpdatePhotonicFenceKillingRate( float rate ) = 0;
        virtual void UpdateArtificialDietAttractionRate( float rate ) = 0;
        virtual void UpdateArtificialDietKillingRate( float rate ) = 0;
        virtual void UpdateInsecticidalDrugKillingProbability( float prob ) = 0;
    };

    struct IBitingRisk : public ISupports
    {
        virtual void UpdateRelativeBitingRate( float rate ) = 0;
    };

}
