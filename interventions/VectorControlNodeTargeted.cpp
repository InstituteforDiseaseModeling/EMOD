/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "VectorControlNodeTargeted.h"

#include "Exceptions.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeVectorEventContext.h" // for INodeVectorInterventionEffectsApply methods
#include "SimulationConfig.h"

static const char * _module = "VectorControlNodeTargeted";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleVectorControlNode)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleVectorControlNode)

    //IMPLEMENT_FACTORY_REGISTERED(SimpleVectorControlNode) // don't register unusable base class
    IMPLEMENT_FACTORY_REGISTERED(Larvicides)
    IMPLEMENT_FACTORY_REGISTERED(SpaceSpraying)
    IMPLEMENT_FACTORY_REGISTERED(SpatialRepellent)
    IMPLEMENT_FACTORY_REGISTERED(ArtificialDiet)
    IMPLEMENT_FACTORY_REGISTERED(InsectKillingFence)
    IMPLEMENT_FACTORY_REGISTERED(SugarTrap)
    IMPLEMENT_FACTORY_REGISTERED(OvipositionTrap)
    IMPLEMENT_FACTORY_REGISTERED(OutdoorRestKill)
    IMPLEMENT_FACTORY_REGISTERED(AnimalFeedKill)

    bool SimpleVectorControlNode::Configure( const Configuration * inputJson )
    {
        // TODO: consider to what extent we want to pull the decay constants out of here as well
        //       in particular, for spatial repellents where there is reduction but not killing, the primary constant is un-used in BOX and DECAY (but not BOXDECAY)
        //       whereas, oviposition traps only have a killing effect.  (ERAD-599)
        initConfig( "Durability_Time_Profile", durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Time_Profile", VCN_Durability_Time_Profile_DESC_TEXT, MDD_ENUM_ARGS(InterventionDurabilityProfile)) );
        initConfigTypeMap("Primary_Decay_Time_Constant", &primary_decay_time_constant, VCN_Primary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 0);
        initConfigTypeMap("Secondary_Decay_Time_Constant", &secondary_decay_time_constant, VCN_Secondary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 0);
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, VCN_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10.0);

        return JsonConfigurable::Configure( inputJson );
    }

    SimpleVectorControlNode::SimpleVectorControlNode() 
        : killing(0)
        , reduction(0)
        , habitat_target(VectorHabitatType::ALL_HABITATS)
        , primary_decay_time_constant(0)
        , secondary_decay_time_constant(0)
        , invic(NULL)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
    }

    SimpleVectorControlNode::~SimpleVectorControlNode()
    {
    }

    void SimpleVectorControlNode::SetContextTo( INodeEventContext *context )
    {
        if (s_OK != context->QueryInterface(GET_IID(INodeVectorInterventionEffectsApply), (void**)&invic) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "INodeVectorInterventionEffectsApply", "INodeEventContext" );
        }
    }

    bool SimpleVectorControlNode::Distribute( INodeEventContext *pNodeContext, IEventCoordinator2 *pEC )
    {
        // Just one of each of these allowed
        pNodeContext->PurgeExisting( typeid(*this).name() ); // hmm?  let's come back to this and query the right interfaces everywhere.
        return BaseNodeIntervention::Distribute( pNodeContext, pEC );
    }
    
    float SimpleVectorControlNode::GetKilling() const
    {
        return killing;
    }

    float SimpleVectorControlNode::GetReduction() const
    {
        return reduction;
    }

    VectorHabitatType::Enum SimpleVectorControlNode::GetHabitatTarget() const
    {
        return habitat_target;
    }

    void SimpleVectorControlNode::Update( float dt )
    {
        if(durability_time_profile == (int)(InterventionDurabilityProfile::BOXDECAYDURABILITY))
        {
            if(primary_decay_time_constant > 0)
            {
                primary_decay_time_constant -= dt;
            }
            else
            {
                if(secondary_decay_time_constant > dt)
                {
                    killing *= (1-dt/secondary_decay_time_constant);
                    reduction *= (1-dt/secondary_decay_time_constant);
                }
                else
                {
                    killing = 0;
                    reduction = 0;
                }
            }
        }
        else if(durability_time_profile == (int)(InterventionDurabilityProfile::DECAYDURABILITY))
        {
            if(primary_decay_time_constant > dt)
                killing *= (1-dt/primary_decay_time_constant);
            else
                killing = 0;

            if(secondary_decay_time_constant > dt)
                reduction *= (1-dt/secondary_decay_time_constant);
            else
                reduction = 0;
        }
        else if(durability_time_profile == (int)(InterventionDurabilityProfile::BOXDURABILITY))
        {
            primary_decay_time_constant -= dt;
            if(primary_decay_time_constant < 0)
                killing = 0;

            secondary_decay_time_constant -= dt;
            if(secondary_decay_time_constant < 0)
                reduction = 0;
        }

        ApplyEffects();
    }

    void SimpleVectorControlNode::ApplyEffects()
    {
        // NO-OP
        LOG_WARN( "ApplyEffects called in SimpleVectorControlNode base class which is no-op. No overloaded ApplyEffects for this class???\n" );
    }

    //--------------------------------------------- Larvicides ---------------------------------------------

    bool Larvicides::Configure( const Configuration * inputJson )
    {
        initConfig( "Habitat_Target", habitat_target, inputJson, MetadataDescriptor::Enum("Habitat_Target", LV_Habitat_Target_DESC_TEXT, MDD_ENUM_ARGS(VectorHabitatType)) );
        initConfigTypeMap("Killing", &killing, LV_Killing_DESC_TEXT, 0, 1, 0);
        initConfigTypeMap("Reduction", &reduction, LV_Reduction_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void Larvicides::ApplyEffects()
    {
        if( invic )
        {
            invic->UpdateLarvalKilling( GetHabitatTarget(), GetKilling() );
            // Additional habitat reduction for breeding sites poisoned by vector-transported larvicides:
            //  Devine, Perea, Killeen, et al. (2009) PNAS vol. 106 no. 28 11530
            //  Devine and Killeen (2010) Malaria Journal 9:142
            invic->UpdateLarvalHabitatReduction( GetHabitatTarget(), GetReduction() ); // TODO: is this adequate for current purposes?  in particular, this doesn't have any dynamic dependence on the vector population size.
        }
    }

    //--------------------------------------------- SpaceSpraying ---------------------------------------------

    bool SpaceSpraying::Configure( const Configuration * inputJson )
    {
        initConfig( "Habitat_Target", habitat_target, inputJson, MetadataDescriptor::Enum("Habitat_Target", SS_Habitat_Target_DESC_TEXT, MDD_ENUM_ARGS(VectorHabitatType)) );
        initConfigTypeMap("Reduction", &reduction, SS_Reduction_DESC_TEXT, 0, 1, 0);
        initConfig( "Spray_Kill_Target", kill_target, inputJson, MetadataDescriptor::Enum("Spray_Kill_Target", SS_Kill_Target_DESC_TEXT, MDD_ENUM_ARGS(SpaceSprayTarget)) );
        initConfigTypeMap("Killing", &killing, SS_Killing_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void SpaceSpraying::ApplyEffects()
    {
        if( invic )
        {
            invic->UpdateLarvalHabitatReduction( GetHabitatTarget(), GetReduction() );
            switch( (int)GetKillTarget() )
            {
                case SpaceSprayTarget::SpaceSpray_FemalesOnly:
                    invic->UpdateOutdoorKilling( GetKilling() );
                    break;

                case SpaceSprayTarget::SpaceSpray_MalesOnly:
                    invic->UpdateOutdoorKillingMale( GetKilling() );
                    break;
            
                case SpaceSprayTarget::SpaceSpray_FemalesAndMales:
                    invic->UpdateOutdoorKilling( GetKilling() );
                    invic->UpdateOutdoorKillingMale( GetKilling() );
                    break;
                break;

                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "GetKillTarget()", GetKillTarget(), SpaceSprayTarget::pairs::lookup_key(GetKillTarget()) );
                break;
            }
        }
    }

    SpaceSprayTarget::Enum SpaceSpraying::GetKillTarget() const
    {
        return kill_target;
    }
    
    //--------------------------------------------- SpatialRepellent ---------------------------------------------

    bool SpatialRepellent::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Repellency", &reduction, SR_Repellency_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void SpatialRepellent::ApplyEffects()
    {
        if( invic )
        {
            invic->UpdateVillageSpatialRepellent( GetReduction() );
        }
    }

    //--------------------------------------------- ArtificialDiet ---------------------------------------------

    bool ArtificialDiet::Configure( const Configuration * inputJson )
    {
        initConfig( "Artificial_Diet_Target", attraction_target, inputJson, MetadataDescriptor::Enum("Artificial_Diet_Target", AD_Target_DESC_TEXT, MDD_ENUM_ARGS(ArtificialDietTarget)) );
        initConfigTypeMap("Attraction", &reduction, AD_Attraction_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void ArtificialDiet::ApplyEffects()
    {
        if( invic )
        {
            switch( (int)GetAttractionTarget() )
            {
                case ArtificialDietTarget::AD_WithinVillage:
                    invic->UpdateADIVAttraction( GetReduction() );
                    break;

                case ArtificialDietTarget::AD_OutsideVillage:
                    invic->UpdateADOVAttraction( GetReduction() );
                    break;

                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "GetAttractionTarget()", GetAttractionTarget(), ArtificialDietTarget::pairs::lookup_key(GetAttractionTarget()) );
            }
        }    
    }

    ArtificialDietTarget::Enum ArtificialDiet::GetAttractionTarget() const
    {
        return attraction_target;
    }

    //--------------------------------------------- InsectKillingFence ---------------------------------------------

    bool InsectKillingFence::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Killing", &killing, VCN_Killing_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void InsectKillingFence::ApplyEffects()
    {
        if( invic )
        {
            invic->UpdatePFVKill( GetKilling() );
        }
    }

    //--------------------------------------------- SugarTrap ---------------------------------------------

    bool SugarTrap::Configure( const Configuration * inputJson )
    {
        if( !JsonConfigurable::_dryrun )
        {
            VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE(SimulationConfig)->vector_sampling_type;
            if ( vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_NUMBER || vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_PERCENT )
            {
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Explicit sugar feeding only implemented in individual-mosquito model, not in cohort model." );
            }
            if ( GET_CONFIGURABLE(SimulationConfig)->vector_sugar_feeding == VectorSugarFeeding::VECTOR_SUGAR_FEEDING_NONE )
            {
                LOG_WARN("Distributing SugarTrap with vector sugar-feeding frequency set to VECTOR_SUGAR_FEEDING_NONE.\n");
            }
        }

        initConfigTypeMap("Killing", &killing, VCN_Killing_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void SugarTrap::ApplyEffects()
    {
        if( invic )
        {
            invic->UpdateSugarFeedKilling( GetKilling() );
        }
    }

    //--------------------------------------------- OvipositionTrap ---------------------------------------------

    bool OvipositionTrap::Configure( const Configuration * inputJson )
    {
        if( !JsonConfigurable::_dryrun )
        {
            VectorSamplingType::Enum vector_sampling_type = GET_CONFIGURABLE(SimulationConfig)->vector_sampling_type;
            if (vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_NUMBER || vector_sampling_type == VectorSamplingType::VECTOR_COMPARTMENTS_PERCENT)
            {
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Explicit oviposition only implemented in individual-mosquito model, not in cohort model." );
            }
        }

        initConfig( "Habitat_Target", habitat_target, inputJson, MetadataDescriptor::Enum("Habitat_Target", OT_Habitat_Target_DESC_TEXT, MDD_ENUM_ARGS(VectorHabitatType)) );
        initConfigTypeMap("Killing", &killing, OT_Killing_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void OvipositionTrap::ApplyEffects()
    {
        if( invic )
        {
            invic->UpdateOviTrapKilling( GetHabitatTarget(), GetKilling() );
        }
    }

    //--------------------------------------------- OutdoorRestKill ---------------------------------------------

    bool OutdoorRestKill::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Killing", &killing, VCN_Killing_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void OutdoorRestKill::ApplyEffects()
    {
        if( invic )
        {
           invic->UpdateOutdoorRestKilling( GetKilling() );
        }
    }

    //--------------------------------------------- AnimalFeedKill ---------------------------------------------

    bool AnimalFeedKill::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Killing", &killing, AFK_Killing_DESC_TEXT, 0, 1, 0);
        return SimpleVectorControlNode::Configure( inputJson );
    }

    void AnimalFeedKill::ApplyEffects()
    {
        if( invic )
        {
            invic->UpdateAnimalFeedKilling( GetKilling() );
        }
    }
}

// This shows how to do serialization from outside the class.
#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::SimpleVectorControlNode)

namespace Kernel {

REGISTER_SERIALIZATION_VOID_CAST(SimpleVectorControlNode, INodeDistributableIntervention);

template<class Archive>
void serialize(Archive &ar, SimpleVectorControlNode& vcn, const unsigned int v)
{
    ar & vcn.durability_time_profile;
    ar & vcn.killing;
    ar & vcn.reduction;
    ar & vcn.habitat_target;
    ar & vcn.primary_decay_time_constant;
    ar & vcn.secondary_decay_time_constant;
    // TODO: if we put some functionality into a BaseNodeIntervention, then we may need to serialize that here
}
}
#endif
