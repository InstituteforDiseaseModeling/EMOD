/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "IdmApi.h"
#include "BoostLibWrapper.h"
#include "IdmApi.h"

#include "ISupports.h"
#include "SimpleTypemapRegistration.h" // for BEGIN/END_PERSIST
#include "Configure.h"                 // for JsonConfigurable
#include "InterventionEnums.h"         // for IndividualEventTriggerType enum
#include "VectorEnums.h"               // for larval habitat targets 
#include "MalariaDrugTypeParameters.h" // REALLY don't want to have to do this.
#include "InterventionValidator.h"
#ifdef ENABLE_TB
#include "TBDrugTypeParameters.h" //Copying the malariadrugtypeparameters
#endif



namespace Kernel
{
    struct IIndividualHumanContext;
    struct IIndividualHumanEventContext;

    struct IDMAPI ICampaignCostObserver : ISupports
    {
        virtual void notifyCampaignExpenseIncurred( float expenseIncurred, const IIndividualHumanEventContext * pIndiv ) = 0;
        virtual void notifyCampaignEventOccurred( /*const*/ ISupports * pDistributedIntervention, /*const*/ ISupports * pDistributor, /*const*/ IIndividualHumanContext * pDistributeeIndividual ) = 0;
        virtual ~ICampaignCostObserver() {}

    };

    struct IIndividualHumanInterventionsContext;

    struct IDMAPI IDistributableIntervention : public ISupports
    {
        // Distribute transfers ownership of this object to the context if it succeeds, the context becomes responsible for freeing it
        // returns false if cannot distribute to the individual represented by this context, for whatever reason
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO ) = 0;
        virtual void SetContextTo(IIndividualHumanContext *context) = 0;
        virtual void Update(float dt) = 0;
        virtual bool Expired() = 0;
        virtual void ValidateSimType( const std::string& simTypeStr ) = 0;
        virtual IDistributableIntervention * Clone()  = 0;

        virtual ~IDistributableIntervention() { }
    };

    struct IIndividualHumanInterventionsContext : public ISupports
    {
        // specific helpers for interoperating with the interventions container might go here
        virtual void SetContextTo(IIndividualHumanContext *context) = 0;
        virtual IIndividualHumanContext* GetParent() = 0;
        virtual std::list<IDistributableIntervention*> GetInterventionsByType(const std::string &type_name) = 0;
        virtual void PurgeExisting( const std::string &iv_name ) = 0;
    };

    struct INodeEventContext;
    struct IEventCoordinator2;

    struct IDMAPI INodeDistributableIntervention : public ISupports
    {
        // Distribute transfers ownership of this object to the context if it succeeds, the context becomes responsible for freeing it
        // returns false if cannot distribute to the individual represented by this context, for whatever reason
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC = NULL ) = 0; 
        virtual void SetContextTo(INodeEventContext *context) = 0;
        virtual void Update(float dt) = 0;
        virtual void ValidateSimType( const std::string& simTypeStr ) = 0;

#if USE_JSON_SERIALIZATION
        // For JSON serialization
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const = 0 ;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper ) = 0;
#endif

        virtual ~INodeDistributableIntervention() { }
    };

    struct IVectorInterventionsEffects : public ISupports
    {
        virtual float GetDieBeforeFeeding() = 0;
        virtual float GetHostNotAvailable() = 0;
        virtual float GetDieDuringFeeding() = 0;
        virtual float GetDiePostFeeding() = 0;
        virtual float GetSuccessfulFeedHuman() = 0;
        virtual float GetSuccessfulFeedAD() = 0;
        virtual float GetOutdoorDieBeforeFeeding() = 0;
        virtual float GetOutdoorHostNotAvailable() = 0;
        virtual float GetOutdoorDieDuringFeeding() = 0;
        virtual float GetOutdoorDiePostFeeding() = 0;
        virtual float GetOutdoorSuccessfulFeedHuman() = 0;
        virtual float GetblockIndoorVectorAcquire() = 0;
        virtual float GetblockIndoorVectorTransmit() = 0;
        virtual float GetblockOutdoorVectorAcquire() = 0;
        virtual float GetblockOutdoorVectorTransmit() = 0;
        virtual ~IVectorInterventionsEffects() { }
    };

    struct INodeVectorInterventionEffects : public ISupports 
    {
        virtual float GetLarvalKilling(VectorHabitatType::Enum) = 0;
        virtual float GetLarvalHabitatReduction(VectorHabitatType::Enum) = 0;
        virtual float GetVillageSpatialRepellent() = 0;
        virtual float GetADIVAttraction() = 0;
        virtual float GetADOVAttraction() = 0;
        virtual float GetPFVKill() = 0;
        virtual float GetOutdoorKilling() = 0;
        virtual float GetOutdoorKillingMale() = 0;
        virtual float GetSugarFeedKilling() = 0;
        virtual float GetOviTrapKilling(VectorHabitatType::Enum) = 0;
        virtual float GetAnimalFeedKilling() = 0;
        virtual float GetOutdoorRestKilling() = 0;
    };


//#ifdef ENABLE_HIV
    struct IHIVDrugEffects : public ISupports
    {
        virtual float GetDrugInactivationRate() = 0;
        virtual float GetDrugClearanceRate() = 0;
        virtual ~IHIVDrugEffects() { }
    };
//#endif

#ifdef ENABLE_POLIO
    class IPolioVaccine;

    struct IPolioVaccineEffects : public ISupports
    {
        virtual std::list<IPolioVaccine*>& GetNewVaccines() = 0; 
        virtual void ClearNewVaccines() = 0;
        virtual ~IPolioVaccineEffects() { }
    };

    struct IPolioDrugEffects : public ISupports
    {
        virtual float get_titer_efficacy() const = 0;
        virtual float get_infection_duration_efficacy() const = 0;

        virtual ~IPolioDrugEffects() { }
    };
#endif

    class IInterventionConsumer : public ISupports
    {
        public:
        virtual bool GiveIntervention( IDistributableIntervention * pIV ) = 0;
    };

    class IDMAPI INodeInterventionConsumer : public ISupports
    {
        public:
        virtual bool GiveIntervention( INodeDistributableIntervention * pIV ) = 0;
    };

    struct IDMAPI IBaseIntervention : public ISupports
    {
        virtual float GetCostPerUnit() const = 0;
    };

    struct IIndividualHumanEventContext;

    struct IDMAPI BaseIntervention : public IBaseIntervention, public JsonConfigurable, public IDistributableIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        virtual float GetCostPerUnit() const { return cost_per_unit; }
        virtual bool Expired();
        virtual void ValidateSimType( const std::string& simTypeStr );

    protected:
        BaseIntervention();
        BaseIntervention( const BaseIntervention& );
        virtual ~BaseIntervention();
        virtual bool Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO );

        float cost_per_unit;
        bool expired;

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        void serialize_inner(Archive &ar, const unsigned int v);
#endif
    };

    struct BaseNodeIntervention : public IBaseIntervention, public JsonConfigurable, public INodeDistributableIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        virtual float GetCostPerUnit() const { return cost_per_unit; }
        virtual bool Expired();
        virtual void ValidateSimType( const std::string& simTypeStr );

    protected:
        BaseNodeIntervention();
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC = NULL ); 

        float cost_per_unit;
        bool expired;

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        void serialize_inner(Archive &ar, const unsigned int v)
        {
            ar & cost_per_unit;
            ar & expired;
        }
#endif
    };

    struct IDMAPI IIndividualEventObserver : public ISupports
    {
        virtual ~IIndividualEventObserver() { }; // for cleanup via interface pointer
        virtual bool notifyOnEvent(IIndividualHumanEventContext *context, const std::string& StateChange) = 0;

#if USE_JSON_SERIALIZATION
        // For JSON serialization
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const = 0;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper ) = 0;
#endif
    };

    // We're not liking these names anymore. TODO: Change to something more semantically useful
    struct IDMAPI INodeTriggeredInterventionConsumer : public ISupports
    {
        virtual void RegisterNodeEventObserver(IIndividualEventObserver* NodeEventObserver, const IndividualEventTriggerType::Enum &trigger ) = 0;
        virtual void UnregisterNodeEventObserver(IIndividualEventObserver* NodeEventObserver, const IndividualEventTriggerType::Enum &trigger ) = 0;
        virtual void TriggerNodeEventObservers(IIndividualHumanEventContext* pIndiv, const IndividualEventTriggerType::Enum &StateChange) = 0;
        virtual void RegisterNodeEventObserverByString( IIndividualEventObserver *pIEO, const std::string &trigger ) = 0;
        virtual void UnregisterNodeEventObserverByString( IIndividualEventObserver *pIEO, const std::string &trigger ) = 0;
        virtual void TriggerNodeEventObserversByString( IIndividualHumanEventContext *ihec, const std::string &trigger ) = 0;
    };
}
