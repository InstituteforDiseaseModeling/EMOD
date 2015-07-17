/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "HumanHostSeekingTrap.h"

#include <typeinfo>

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "VectorInterventionsContainer.h"  // for IVectorInterventionEffectsSetter methods

static const char* _module = "HumanHostSeekingTrap";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HumanHostSeekingTrap)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(HumanHostSeekingTrap)

    IMPLEMENT_FACTORY_REGISTERED(HumanHostSeekingTrap)
    
    HumanHostSeekingTrap::HumanHostSeekingTrap()
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap( "Attract_Rate", &current_attractrate, HST_Attract_Rate_DESC_TEXT, 0.0, 1.0, 0.5 );
        initConfigTypeMap( "Killing_Rate", &current_killingrate, HST_Killing_Rate_DESC_TEXT, 0.0, 1.0, 0.5 );
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, HST_Cost_To_Consumer_DESC_TEXT, 0, 999999, 3.75 );
        initConfigTypeMap( "Primary_Decay_Time_Constant", &primary_decay_time_constant, HST_Primary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 3650 );
        initConfigTypeMap( "Secondary_Decay_Time_Constant", &secondary_decay_time_constant, HST_Secondary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 3650 );
    }

    bool
    HumanHostSeekingTrap::Configure(
        const Configuration * inputJson
    )
    {
        initConfig( "Durability_Time_Profile", durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Time_Profile", HST_Durability_Time_Profile_DESC_TEXT, MDD_ENUM_ARGS(InterventionDurabilityProfile)) );
        return JsonConfigurable::Configure( inputJson );
    }

    bool
    HumanHostSeekingTrap::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&ivies) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorInterventionEffectsSetter", "IIndividualHumanInterventionsContext" );
        }
        context->PurgeExisting( typeid(*this).name() );
        return BaseIntervention::Distribute( context, pCCO );
    }

    void HumanHostSeekingTrap::Update( float dt )
    {
        if (durability_time_profile == InterventionDurabilityProfile::BOXDECAYDURABILITY/*(int)(InterventionDurabilityProfile::BOXDECAYDURABILITY)*/)
        {
            if(primary_decay_time_constant>0)
            {
                primary_decay_time_constant-=dt;
            }
            else
            {
                if(secondary_decay_time_constant > dt)
                {
                    current_killingrate *= (1-dt/secondary_decay_time_constant);
                    current_attractrate *= (1-dt/secondary_decay_time_constant);
                }
                else
                {
                    current_killingrate = 0;
                    current_attractrate = 0;
                }
            }
        }
        else if (durability_time_profile == (int)(InterventionDurabilityProfile::DECAYDURABILITY))
        {
            if(primary_decay_time_constant > dt)
                current_killingrate *= (1-dt/primary_decay_time_constant);
            else
                current_killingrate = 0;

            if(secondary_decay_time_constant > dt)
                current_attractrate *= (1-dt/secondary_decay_time_constant);
            else
                current_attractrate = 0;
        }
        else if(durability_time_profile == (int)(InterventionDurabilityProfile::BOXDURABILITY))
        {
            primary_decay_time_constant -= dt;
            if(primary_decay_time_constant < 0)
                current_killingrate = 0;

            secondary_decay_time_constant -= dt;
            if(secondary_decay_time_constant < 0)
                current_attractrate = 0;
        }

        // Effects of human host-seeking trap are updated with indoor-home artificial-diet interfaces in VectorInterventionsContainer::Update.
        // Attraction rate diverts indoor feeding attempts from humans to trap; killing rate kills a fraction of diverted feeding attempts.
        ivies->UpdateArtificialDietAttractionRate( current_attractrate );
        ivies->UpdateArtificialDietKillingRate( current_killingrate );
    }

    void HumanHostSeekingTrap::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&ivies) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorInterventionEffectsSetter", "IIndividualHumanContext" );
        }
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT(Kernel::HumanHostSeekingTrap)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, HumanHostSeekingTrap& obj, const unsigned int v)
    {
        boost::serialization::void_cast_register<HumanHostSeekingTrap, IDistributableIntervention>();
        ar & obj.current_attractrate;
        ar & obj.current_killingrate;
        ar & obj.primary_decay_time_constant;
        ar & obj.secondary_decay_time_constant;
        ar & obj.durability_time_profile;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(obj);
    }
}
#endif