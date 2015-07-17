/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "PropertyValueChanger.h"
#include "Contexts.h"
#include "Debug.h" // for release_assert
#include "RANDOM.h"
#include "Common.h"             // for INFINITE_TIME
#include "InterventionEnums.h"  // for InterventionDurabilityProfile, ImmunoglobulinType, etc.
#include "Individual.h"  // has to go?
#include "InterventionsContainer.h"  // for IPropertyValueChangerEffects
#include "MathFunctions.h"  // for IPropertyValueChangerEffects
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)

static const char * _module = "PropertyValueChanger";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(PropertyValueChanger)

    BEGIN_QUERY_INTERFACE_BODY(PropertyValueChanger)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IPropertyValueChanger)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(PropertyValueChanger)

    PropertyValueChanger::PropertyValueChanger()
        :probability(1.0)
        , revert(0.0f)
        , max_duration(0.0f)
        , action_timer(0.0f)
        , reversion_timer(0.0f)
        , ibc(NULL)
    {
        expired = false;
        //std::cout << __FUNCTION__ << ":" << __LINE__ << std::endl;
        // done in base class ctor, not needed here.
        // primary_decay_time_constant...
        // cost_per_unit...
    }

    PropertyValueChanger::~PropertyValueChanger()
    {
        LOG_DEBUG("PropertyValueChanger destructor \n");
    }

    bool
    PropertyValueChanger::Configure(
        const Configuration * inputJson
    )
    {
        target_property_key.constraints = "<demographics>::Defaults.Individual_Properties.*.Property.<keys>"; 
        target_property_value.constraints = "<demographics>::Defaults.Individual_Properties.*.Value.<keys>"; 
        initConfigTypeMap("Target_Property_Key", &target_property_key, PC_Target_Property_Key_DESC_TEXT );
        initConfigTypeMap("Target_Property_Value", &target_property_value, PC_Target_Property_Value_DESC_TEXT );
        initConfigTypeMap("Daily_Probability", &probability, PC_Daily_Probability_DESC_TEXT, 0.0f, 1.0f );
        initConfigTypeMap("Maximum_Duration", &max_duration, PC_Maximum_Duration_DESC_TEXT, -1.0f, FLT_MAX, FLT_MAX);
        initConfigTypeMap("Revert", &revert, PC_Revert_DESC_TEXT, 0.0f, 10000.0f, 0.0f );
        bool ret = JsonConfigurable::Configure( inputJson );
        if( probability < 1.0 )
        {
            action_timer = Probability::getInstance()->fromDistribution( DistributionFunction::EXPONENTIAL_DURATION, probability, 0, 0 );
            if( action_timer > max_duration )
            {
                action_timer = FLT_MAX;
            }
            LOG_DEBUG_F( "Time until property change occurs = %f\n", action_timer );
        }
        return ret;
    }

    bool
    PropertyValueChanger::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IPropertyValueChangerEffects), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IPropertyValueChangerEffects", "IIndividualHumanInterventionsContext" );
        }
        return BaseIntervention::Distribute( context, pCCO );
    }

    void PropertyValueChanger::Update( float dt )
    {
        release_assert( expired == false );
        release_assert( ibc );

        std::string current_prop_value = "";
        if( reversion_timer > 0 )
        {
            reversion_timer -= dt;
            if( reversion_timer <= 0 )
            {
                LOG_DEBUG_F( "Time to revert PropertyValueChanger.\n" );
                probability = 1.0;
            }
        }
        if( probability == 1.0 || action_timer < 0 )
        {
            if( revert )
            {
                // Need to ask individual (parent's parent) for current value of this property
                // TBD: use QI not static cast
                auto props = static_cast<InterventionsContainer*>(ibc)->GetParent()->GetEventContext()->GetProperties();
                current_prop_value = props->find( (std::string) target_property_key )->second;
            }
            ibc->ChangeProperty( target_property_key.c_str(), target_property_value.c_str() );

            //broadcast that the individual changed properties
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            LOG_DEBUG_F( "Individual %d changed property, broadcasting PropertyChange \n", parent->GetSuid().data );
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::PropertyChange );


            if( revert )
            {
                target_property_value = current_prop_value;
                probability = 0.0; // keep it simple for now, reversion is guaranteed
                reversion_timer = revert;
                action_timer= FLT_MAX;
                LOG_DEBUG_F( "Initializing reversion timer to %f\n", reversion_timer );
                revert = 0; // no more reversion from reversion
            }
            else
            {
                expired = true;
            }
        }
        action_timer -= dt;
    }

    void PropertyValueChanger::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        parent = context;
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IPropertyValueChangerEffects), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IPropertyValueChangerEffects", "IIndividualHumanContext" );
        }
    }

    int
    PropertyValueChanger::AddRef()
    {
        return BaseIntervention::AddRef();
    }

    int
    PropertyValueChanger::Release()
    {
        return BaseIntervention::Release();
    }
    
    const char * PropertyValueChanger::GetTargetPropertyValue()
    {
        return target_property_value.c_str();
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::PropertyValueChanger)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, PropertyValueChanger& pc, const unsigned int v)
    {
        boost::serialization::void_cast_register<PropertyValueChanger, IDistributableIntervention>();
        ar & (std::string)pc.target_property_key;
        ar & (std::string)pc.target_property_value;
        ar & pc.probability;
        ar & pc.revert;
        ar & pc.max_duration;
        ar & pc.action_timer;
        ar & pc.reversion_timer;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(pc);
    }
}
#endif
