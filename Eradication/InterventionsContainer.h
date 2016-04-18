/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <list>

#include "BoostLibWrapper.h"
#include "Interventions.h"
#include "Contexts.h"

namespace Kernel
{
    // this container becomes a help implementation member of the relevant IndividualHuman class 
    // it needs to implement consumer interfaces for all the relevant intervention types
    // InterventionsContainer is a generic 

    struct IVaccineConsumer : public ISupports
    {
        virtual void UpdateVaccineAcquireRate( float acq )    = 0;
        virtual void UpdateVaccineTransmitRate( float xmit )  = 0;
        virtual void UpdateVaccineMortalityRate( float mort ) = 0;
    };

    struct IDrugVaccineInterventionEffects : public ISupports
    {
        virtual float GetInterventionReducedAcquire()   const = 0;
        virtual float GetInterventionReducedTransmit()  const = 0;
        virtual float GetInterventionReducedMortality() const = 0;
        virtual ~IDrugVaccineInterventionEffects() { }
    };

    struct IPropertyValueChangerEffects : public ISupports
    {
        virtual void ChangeProperty( const char *property, const char* new_value) = 0;
        virtual ~IPropertyValueChangerEffects() { }
    };
    
    class InterventionsContainer : public IIndividualHumanInterventionsContext, 
                                   public IVaccineConsumer,
                                   public IInterventionConsumer,
                                   public IDrugVaccineInterventionEffects,
                                   public IPropertyValueChangerEffects
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        InterventionsContainer();
        virtual ~InterventionsContainer();

        // IIndividualHumanInterventionsContext
        virtual void SetContextTo(IIndividualHumanContext* context) override;
        virtual IIndividualHumanContext* GetParent() override;
        virtual std::list<IDistributableIntervention*> GetInterventionsByType(const std::string& type_name) override;
        virtual void PurgeExisting( const std::string& iv_name ) override;
        virtual bool ContainsExisting( const std::string &iv_name ) override;

        // IUnknown
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // IVaccineConsumer
        virtual void UpdateVaccineAcquireRate( float acq ) override;
        virtual void UpdateVaccineTransmitRate( float xmit ) override;
        virtual void UpdateVaccineMortalityRate( float mort ) override;

        // IDrugVaccineInterventionEffects
        virtual float GetInterventionReducedAcquire()   const override;
        virtual float GetInterventionReducedTransmit()  const override;
        virtual float GetInterventionReducedMortality() const override;


        // IPropertyValueChangerEffects
        virtual void ChangeProperty( const char *property, const char* new_value) override;

        virtual bool GiveIntervention( IDistributableIntervention * pIV ) override;

        virtual void Update(float dt); // hook to update interventions if they need it

    protected:
        float drugVaccineReducedAcquire;
        float drugVaccineReducedTransmit;
        float drugVaccineReducedMortality;
        std::list<IDistributableIntervention*> interventions;

        virtual void PropagateContextToDependents(); // pass context to interventions if they need it

        IIndividualHumanContext *parent;    // context for this interventions container

    private:
        IDistributableIntervention* GetIntervention( const std::string& iv_name );

        DECLARE_SERIALIZABLE(InterventionsContainer);
    };
}
