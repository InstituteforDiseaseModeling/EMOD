/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "HousingModification.h"

#include <typeinfo>

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "VectorInterventionsContainer.h"  // for IHousingModificationConsumer methods

static const char* _module = "SimpleHousingModification";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(SimpleHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(IRSHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(ScreeningHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(SpatialRepellentHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(ArtificialDietHousingModification)
    IMPLEMENT_FACTORY_REGISTERED(InsectKillingFenceHousingModification)

    bool
    SimpleHousingModification::Configure(
        const Configuration * inputJson
    )
    {
        initConfig( "Durability_Time_Profile", durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Time_Profile", HM_Durability_Time_Profile_DESC_TEXT, MDD_ENUM_ARGS(InterventionDurabilityProfile)) );
        return JsonConfigurable::Configure( inputJson );
    }

    SimpleHousingModification::SimpleHousingModification()
        : durability_time_profile( InterventionDurabilityProfile::BOXDURABILITY )
        , current_blockingrate(0.0f)
        , current_killingrate(0.0f)
        , primary_decay_time_constant(0.0f)
        , secondary_decay_time_constant(0.0f)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap("Primary_Decay_Time_Constant", &primary_decay_time_constant, HM_Primary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 3650);
        initConfigTypeMap("Secondary_Decay_Time_Constant", &secondary_decay_time_constant, HM_Secondary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 3650);
        initConfigTypeMap("Blocking_Rate", &current_blockingrate, HM_Blocking_Rate_DESC_TEXT, 0.0, 1.0, 1.0);
        initConfigTypeMap("Killing_Rate", &current_killingrate, HM_Killing_Rate_DESC_TEXT, 0.0, 1.0, 1.0);
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, HM_Cost_To_Consumer_DESC_TEXT, 0, 999999, 8.0);
    }

    bool
    SimpleHousingModification::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IHousingModificationConsumer), (void**)&ihmc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHousingModificationConsumer", "IIndividualHumanInterventionsContext" );
        }
        context->PurgeExisting( typeid(*this).name() );
        return BaseIntervention::Distribute( context, pCCO );
    }

    void SimpleHousingModification::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        LOG_DEBUG("SimpleHousingModification::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHousingModificationConsumer), (void**)&ihmc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHousingModificationConsumer", "IIndividualHumanContext" );
        }

    }

    void SimpleHousingModification::Update( float dt )
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
                    current_killingrate *= (1-dt/secondary_decay_time_constant);
                    current_blockingrate *= (1-dt/secondary_decay_time_constant);
                }
                else
                {
                    current_killingrate = 0;
                    current_blockingrate = 0;
                }
            }
        }
        else if(durability_time_profile == (int)(InterventionDurabilityProfile::DECAYDURABILITY))
        {
            if(primary_decay_time_constant > dt)
                current_killingrate *= (1-dt/primary_decay_time_constant);
            else
                current_killingrate = 0;

            if(secondary_decay_time_constant > dt)
                current_blockingrate *= (1-dt/secondary_decay_time_constant);
            else
                current_blockingrate = 0;
        }
        else if(durability_time_profile == (int)(InterventionDurabilityProfile::BOXDURABILITY))
        {
            primary_decay_time_constant -= dt;
            if(primary_decay_time_constant < 0)
                current_killingrate = 0;

            secondary_decay_time_constant -= dt;
            if(secondary_decay_time_constant < 0)
                current_blockingrate = 0;
        }
        if( ihmc )
        {
            ihmc->ApplyHouseBlockingProbability( current_blockingrate );
            ihmc->UpdateProbabilityOfScreenKilling( current_killingrate );
        }
        else
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "ihmc" );
        }
    }

    BEGIN_QUERY_INTERFACE_BODY(SimpleHousingModification)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleHousingModification)
/*
    Kernel::QueryResult SimpleHousingModification::QueryInterface( iid_t iid, void **ppinstance )
    {
        assert(ppinstance);

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(IHousingModification)) 
            foundInterface = static_cast<IHousingModification*>(this);
        // -->> add support for other I*Consumer interfaces here <<--      
      
        else if ( iid == GET_IID(ISupports)) 
            foundInterface = static_cast<ISupports*>(static_cast<IHousingModification*>(this));
        else
            foundInterface = 0;

        QueryResult status;
        if ( !foundInterface )
            status = e_NOINTERFACE;
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }*/
}

// This shows how to do serialization from outside the class.
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::SimpleHousingModification)
BOOST_CLASS_EXPORT(Kernel::IRSHousingModification)
BOOST_CLASS_EXPORT(Kernel::ScreeningHousingModification)
BOOST_CLASS_EXPORT(Kernel::SpatialRepellentHousingModification)
BOOST_CLASS_EXPORT(Kernel::ArtificialDietHousingModification)
BOOST_CLASS_EXPORT(Kernel::InsectKillingFenceHousingModification)

namespace Kernel {

    template<class Archive>
    void serialize(Archive &ar, SimpleHousingModification& hm, const unsigned int v)
    {
        static const char * _module = "SimpleHousingModification";
        LOG_DEBUG("(De)serializing SimpleHousingModification\n");

        ar & hm.durability_time_profile;
        ar & hm.current_blockingrate;
        ar & hm.current_killingrate;
        ar & hm.primary_decay_time_constant;
        ar & hm.secondary_decay_time_constant;
        ar & boost::serialization::base_object<BaseIntervention>(hm);
    }

    template<class Archive>
    void serialize(Archive &ar, IRSHousingModification& hm, const unsigned int v)
    {
        boost::serialization::void_cast_register<IRSHousingModification, IDistributableIntervention>();
        ar & boost::serialization::base_object<SimpleHousingModification>(hm);
    }

    template<class Archive>
    void serialize(Archive &ar, ScreeningHousingModification& hm, const unsigned int v)
    {
        boost::serialization::void_cast_register<ScreeningHousingModification, IDistributableIntervention>();
        ar & boost::serialization::base_object<SimpleHousingModification>(hm);
    }

    template<class Archive>
    void serialize(Archive &ar, SpatialRepellentHousingModification& hm, const unsigned int v)
    {
        boost::serialization::void_cast_register<SpatialRepellentHousingModification, IDistributableIntervention>();
        ar & boost::serialization::base_object<SimpleHousingModification>(hm);
    }

    template<class Archive>
    void serialize(Archive &ar, ArtificialDietHousingModification& hm, const unsigned int v)
    {
        boost::serialization::void_cast_register<ArtificialDietHousingModification, IDistributableIntervention>();
        ar & boost::serialization::base_object<SimpleHousingModification>(hm);
    }

    template<class Archive>
    void serialize(Archive &ar, InsectKillingFenceHousingModification& hm, const unsigned int v)
    {
        boost::serialization::void_cast_register<InsectKillingFenceHousingModification, IDistributableIntervention>();
        ar & boost::serialization::base_object<SimpleHousingModification>(hm);
    }
}
#endif

