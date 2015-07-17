/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "SimpleTypemapRegistration.h"
#include "Contexts.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"

namespace Kernel
{
    struct IVaccineConsumer; 
    struct ICampaignCostObserver; 

    struct IVaccine : public ISupports
    {
        virtual void  ApplyVaccineTake()                = 0;
        virtual ~IVaccine() { } // needed for cleanup via interface pointer
    };

    class ISimpleVaccine : public ISupports
    {
    public:
        virtual int   GetVaccineType()            const = 0;
    };

    class SimpleVaccine : public IVaccine, public ISimpleVaccine, public BaseIntervention
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleVaccine, IDistributableIntervention)

    public:
        SimpleVaccine();
        virtual ~SimpleVaccine() { }
        virtual int AddRef() { return BaseIntervention::AddRef(); }
        virtual int Release() { return BaseIntervention::Release(); }
        bool Configure( const Configuration* pConfig );

        // IDistributableIntervention
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pCCO );
        virtual void SetContextTo(IIndividualHumanContext *context);
        virtual void Update(float dt);

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        // IVaccine
        virtual int   GetVaccineType()            const;
        virtual void  ApplyVaccineTake(); 

    protected:
        // context for this intervention--does not need to be reset upon migration, it is just for GiveVaccine()
        IIndividualHumanContext *parent;

    protected:
        int   vaccine_type;
        float vaccine_take;
        float current_reducedacquire;
        float current_reducedtransmit;
        float current_reducedmortality;
        InterventionDurabilityProfile::Enum durability_time_profile;
        float primary_decay_time_constant;
        float secondary_decay_time_constant;
        IVaccineConsumer * ivc; // interventions container

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        template<class Archive>
        friend void serialize(Archive &ar, SimpleVaccine& vacc, const unsigned int v);
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
     // IJsonSerializable Interfaces
     virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
     virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
#ifdef WIN32
BOOST_CLASS_EXPORT_KEY(Kernel::SimpleVaccine)
#endif
#endif
