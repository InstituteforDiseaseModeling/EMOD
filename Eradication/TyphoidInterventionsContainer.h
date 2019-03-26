/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#ifdef ENABLE_TYPHOID
#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "InterventionsContainer.h"
#include "Types.h"

namespace Kernel
{
    // this container becomes a help implementation member of the relevant IndividualHuman class 
    // it needs to implement consumer interfaces for all the relevant intervention types
    struct ITyphoidVaccineEffectsApply : public ISupports
    {
        virtual void ApplyReducedSheddingEffect( float rate ) = 0;
        virtual void ApplyReducedDoseEffect( float rate ) = 0;
        virtual void ApplyReducedNumberExposuresEffect( float rate ) = 0;
        virtual void ApplyClearance( ProbabilityNumber clearanceProbability ) = 0;
    };

    class TyphoidInterventionsContainer : public InterventionsContainer,
                                          public ITyphoidVaccineEffectsApply
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        TyphoidInterventionsContainer();
        virtual ~TyphoidInterventionsContainer();

        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // Typhoid 'Vaccine' Apply/Update/Setter functions
        virtual void ApplyReducedSheddingEffect( float rate ) override; 
        virtual void ApplyReducedDoseEffect( float rate ) override;
        virtual void ApplyReducedNumberExposuresEffect( float rate ) override;

        // Typhoid 'Vaccine' Getter functions, x6, explicit -- TODO these should be in an interface!
        virtual float GetContactDepositAttenuation() const; 
        virtual float GetEnviroDepositAttenuation() const;
        virtual float GetContactExposuresAttenuation() const; 
        virtual float GetEnviroExposuresAttenuation() const;
        virtual float GetContactDoseAttenuation() const; 
        virtual float GetEnviroDoseAttenuation() const;


        virtual void ApplyClearance( ProbabilityNumber clearanceProbability = 1.0f ); 
        virtual void Update(float dt) override; // example of intervention timestep update

    protected:
        float current_shedding_attenuation_contact;
        float current_shedding_attenuation_environment;
        float current_dose_attenuation_contact;
        float current_dose_attenuation_environment;
        float current_exposures_attenuation_contact;
        float current_exposures_attenuation_environment;

    private:
    };
}
#endif
