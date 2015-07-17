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
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "Configure.h"

namespace Kernel
{
    class INodeVectorInterventionEffectsApply;

    class SimpleVectorControlNode : public BaseNodeIntervention
    {
        //DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleVectorControlNode, INodeDistributableIntervention) 

    public:        
        SimpleVectorControlNode();
        virtual ~SimpleVectorControlNode();

        // INodeDistributableIntervention
        bool Configure( const Configuration * config );
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC=NULL); 
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual void SetContextTo(INodeEventContext *context);
        virtual void Update(float dt);

    protected:
        virtual void ApplyEffects();

        float GetKilling() const;
        float GetReduction() const;
        VectorHabitatType::Enum GetHabitatTarget() const;

        float killing;
        float reduction;
        VectorHabitatType::Enum habitat_target;
        InterventionDurabilityProfile::Enum durability_time_profile;
        float primary_decay_time_constant;
        float secondary_decay_time_constant;
         
        INodeVectorInterventionEffectsApply *invic;

    private:
#if USE_BOOST_SERIALIZATION
        template<class Archive>
        friend void serialize(Archive &ar, SimpleVectorControlNode& vcn, const unsigned int v);
#endif
    };

    class Larvicides : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, Larvicides, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
    };

    class SpaceSpraying : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SpaceSpraying, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
        SpaceSprayTarget::Enum GetKillTarget() const;

    protected:
        SpaceSprayTarget::Enum kill_target;
    };

    class SpatialRepellent : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SpatialRepellent, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
    };

    class ArtificialDiet : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ArtificialDiet, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
        ArtificialDietTarget::Enum GetAttractionTarget() const;
    protected:
        ArtificialDietTarget::Enum attraction_target;
    };

    class InsectKillingFence : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, InsectKillingFence, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
    };

    class SugarTrap : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SugarTrap, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
    };

    class OvipositionTrap : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, OvipositionTrap, INodeDistributableIntervention) 
        
    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
    };

    class OutdoorRestKill : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, OutdoorRestKill, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
    };

    class AnimalFeedKill : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, AnimalFeedKill, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config );
        virtual void ApplyEffects();
    };
}
