/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "Interventions.h"
#include "InterventionFactory.h"
#include "SimpleTypemapRegistration.h"

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
        virtual void SetContextTo(IIndividualHumanContext* context);
        virtual IIndividualHumanContext* GetParent();
        virtual std::list<IDistributableIntervention*> GetInterventionsByType(const std::string& type_name);
        virtual void PurgeExisting( const std::string& iv_name );

        // IUnknown
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance);

        // IVaccineConsumer
        virtual void UpdateVaccineAcquireRate( float acq );
        virtual void UpdateVaccineTransmitRate( float xmit );
        virtual void UpdateVaccineMortalityRate( float mort );

        // IDrugVaccineInterventionEffects
        virtual float GetInterventionReducedAcquire()   const;
        virtual float GetInterventionReducedTransmit()  const;
        virtual float GetInterventionReducedMortality() const;


        // IPropertyValueChangerEffects
        virtual void ChangeProperty( const char *property, const char* new_value);

        virtual bool GiveIntervention( IDistributableIntervention * pIV );

        virtual void Update(float dt); // hook to update interventions if they need it

    protected:
        float drugVaccineReducedAcquire;
        float drugVaccineReducedTransmit;
        float drugVaccineReducedMortality;
        std::list<IDistributableIntervention*> interventions;

        virtual void PropagateContextToDependents(); // pass context to interventions if they need it

        IIndividualHumanContext *parent;    // context for this interventions container

    private:

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, InterventionsContainer &cont, const unsigned int v);
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
         // IJsonSerializable Interfaces
         virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
         virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };
}
