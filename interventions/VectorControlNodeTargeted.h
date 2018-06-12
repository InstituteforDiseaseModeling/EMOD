/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "VectorEnums.h"
#include "Configure.h"
#include "IWaningEffect.h"

namespace Kernel
{
    ENUM_DEFINE(SpaceSprayTarget,
        ENUM_VALUE_SPEC(SpaceSpray_FemalesOnly       , 11)
        ENUM_VALUE_SPEC(SpaceSpray_MalesOnly         , 12)
        ENUM_VALUE_SPEC(SpaceSpray_FemalesAndMales   , 13)
        ENUM_VALUE_SPEC(SpaceSpray_Indoor            , 14))

    ENUM_DEFINE(ArtificialDietTarget,
        //ENUM_VALUE_SPEC(AD_WithinHouse             , 20) // to be handled as individual rather than node-targeted intervention
        ENUM_VALUE_SPEC(AD_WithinVillage             , 21)
        ENUM_VALUE_SPEC(AD_OutsideVillage            , 22))

    class INodeVectorInterventionEffectsApply;

    class SimpleVectorControlNode : public BaseNodeIntervention
    {
        //DECLARE_FACTORY_REGISTERED(InterventionFactory, SimpleVectorControlNode, INodeDistributableIntervention) 

    public:        
        SimpleVectorControlNode();
        SimpleVectorControlNode( const SimpleVectorControlNode& );
        virtual ~SimpleVectorControlNode();

        // INodeDistributableIntervention
        virtual bool Configure( const Configuration * config ) override;
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC=nullptr) override; 
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) override;
        virtual void SetContextTo(INodeEventContext *context) override;
        virtual void Update(float dt) override;

    protected:
        virtual void ApplyEffects();

        float GetKilling() const;
        float GetReduction() const;
        VectorHabitatType::Enum GetHabitatTarget() const;

        float killing;
        float reduction;
        VectorHabitatType::Enum habitat_target;
        IWaningEffect* killing_effect;
        IWaningEffect* blocking_effect;
         
        INodeVectorInterventionEffectsApply *invic;
    };

    class Larvicides : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, Larvicides, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
    };

    class SpaceSpraying : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SpaceSpraying, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
        SpaceSprayTarget::Enum GetKillTarget() const;

    protected:
        SpaceSprayTarget::Enum kill_target;
    };

    class SpatialRepellent : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SpatialRepellent, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
    };

    class ArtificialDiet : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, ArtificialDiet, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
        ArtificialDietTarget::Enum GetAttractionTarget() const;
    protected:
        ArtificialDietTarget::Enum attraction_target;
    };

    class InsectKillingFence : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, InsectKillingFence, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
    };

    class SugarTrap : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, SugarTrap, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
    };

    class OvipositionTrap : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, OvipositionTrap, INodeDistributableIntervention) 
        
    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
    };

    class OutdoorRestKill : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, OutdoorRestKill, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
    };

    class AnimalFeedKill : public SimpleVectorControlNode
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, AnimalFeedKill, INodeDistributableIntervention) 

    public:
        virtual bool Configure( const Configuration * config ) override;
        virtual void ApplyEffects() override;
    };
}
