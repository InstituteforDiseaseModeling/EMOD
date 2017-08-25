/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>

#include "Interventions.h"
#include "InterventionEnums.h"
#include "InterventionsContainer.h"
#include "VectorContexts.h"

namespace Kernel
{
    struct IBednet;

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

    // This container becomes a help implementation member of the relevant IndividualHuman class.
    // It needs to implement consumer interfaces for all the relevant intervention types.

    class VectorInterventionsContainer : public InterventionsContainer,
                                         public IVectorInterventionsEffects,
                                         public IVectorInterventionEffectsSetter,
                                         public IBednetConsumer,
                                         public IHousingModificationConsumer,
                                         public IIndividualRepellentConsumer
    {
    public:
        VectorInterventionsContainer();
        virtual ~VectorInterventionsContainer();

        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;
        virtual int AddRef() override;
        virtual int Release() override;

        // IBednetConsumer
        virtual void UpdateProbabilityOfBlocking( float prob ) override;
        virtual void UpdateProbabilityOfKilling( float prob ) override;

        // IHousingModificationConsumer
        virtual void ApplyHouseBlockingProbability( float prob ) override;
        virtual void UpdateProbabilityOfScreenKilling( float prob ) override;

        // IIndividualRepellentConsumer
        virtual void UpdateProbabilityOfIndRepBlocking( float prob ) override;
        virtual void UpdateProbabilityOfIndRepKilling( float prob ) override;

        // IVectorInterventionEffectsSetter
        virtual void UpdatePhotonicFenceKillingRate( float rate ) override;
        virtual void UpdateArtificialDietAttractionRate( float rate ) override;
        virtual void UpdateArtificialDietKillingRate( float rate ) override;
        virtual void UpdateInsecticidalDrugKillingProbability( float prob ) override;

        virtual void Update(float dt) override; // example of intervention timestep update

        // IVectorInterventionEffects
        virtual float GetDieBeforeFeeding() override;
        virtual float GetHostNotAvailable() override;
        virtual float GetDieDuringFeeding() override;
        virtual float GetDiePostFeeding() override;
        virtual float GetSuccessfulFeedHuman() override;
        virtual float GetSuccessfulFeedAD() override;
        virtual float GetOutdoorDieBeforeFeeding() override;
        virtual float GetOutdoorHostNotAvailable() override;
        virtual float GetOutdoorDieDuringFeeding() override;
        virtual float GetOutdoorDiePostFeeding() override;
        virtual float GetOutdoorSuccessfulFeedHuman() override;
        virtual float GetblockIndoorVectorAcquire() override;
        virtual float GetblockIndoorVectorTransmit() override;
        virtual float GetblockOutdoorVectorAcquire() override;
        virtual float GetblockOutdoorVectorTransmit() override;

    protected:
        // These are calculated from the values set by the interventions and returned to the model
        float p_block_net;
        float p_kill_ITN;
        float p_penetrate_housingmod;
        float p_kill_IRSprefeed;
        float p_kill_IRSpostfeed;
        float p_block_indrep;
        float p_kill_indrep;
        float p_kill_PFH;
        float p_attraction_ADIH;
        float p_kill_ADIH;
        float p_survive_insecticidal_drug;

        // These are set by interventions
        float pDieBeforeFeeding;
        float pHostNotAvailable;
        float pDieDuringFeeding;
        float pDiePostFeeding;
        float pSuccessfulFeedHuman;
        float pSuccessfulFeedAD;
        float pOutdoorDieBeforeFeeding;
        float pOutdoorHostNotAvailable;
        float pOutdoorDieDuringFeeding;
        float pOutdoorDiePostFeeding;
        float pOutdoorSuccessfulFeedHuman;
        float blockIndoorVectorAcquire;
        float blockIndoorVectorTransmit;
        float blockOutdoorVectorAcquire;
        float blockOutdoorVectorTransmit;

        DECLARE_SERIALIZABLE(VectorInterventionsContainer);
    };
}
