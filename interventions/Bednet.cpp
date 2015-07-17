/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "Bednet.h"

#include <typeinfo>

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "VectorInterventionsContainer.h"  // for IBednetConsumer methods

static const char* _module = "SimpleBednet";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleBednet)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleBednet)

    IMPLEMENT_FACTORY_REGISTERED(SimpleBednet)
    
    SimpleBednet::SimpleBednet()
    {
        initSimTypes( 2, "MALARIA_SIM", "VECTOR_SIM" );
        initConfigTypeMap( "Blocking_Rate", &current_blockingrate, SB_Blocking_Rate_DESC_TEXT, 0.0, 1.0, 0.5 );
        initConfigTypeMap( "Killing_Rate", &current_killingrate, SB_Killing_Rate_DESC_TEXT, 0.0, 1.0, 0.5 );
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, SB_Cost_To_Consumer_DESC_TEXT, 0, 999999, 3.75 );
        initConfigTypeMap( "Primary_Decay_Time_Constant", &primary_decay_time_constant, SB_Primary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 3650);
        initConfigTypeMap( "Secondary_Decay_Time_Constant", &secondary_decay_time_constant, SB_Secondary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 3650);
    }

    bool
    SimpleBednet::Configure(
        const Configuration * inputJson
    )
    {
        initConfig( "Durability_Time_Profile", durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Time_Profile", SB_Durability_Time_Profile_DESC_TEXT, MDD_ENUM_ARGS(InterventionDurabilityProfile)) );
        initConfig( "Bednet_Type", bednet_type, inputJson, MetadataDescriptor::Enum("Bednet_Type", SB_Bednet_Type_DESC_TEXT, MDD_ENUM_ARGS(BednetType)) );
        return JsonConfigurable::Configure( inputJson );
    }

    bool
    SimpleBednet::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IBednetConsumer), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IBednetConsumer", "IIndividualHumanInterventionsContext" );
        }
        context->PurgeExisting( typeid(*this).name() );
        return BaseIntervention::Distribute( context, pCCO );
    }

    void SimpleBednet::Update( float dt )
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
                    current_blockingrate *= (1-dt/secondary_decay_time_constant);
                }
                else
                {
                    current_killingrate = 0;
                    current_blockingrate = 0;
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
        ibc->UpdateProbabilityOfBlocking( current_blockingrate );
        ibc->UpdateProbabilityOfKilling( current_killingrate );
    }

    void SimpleBednet::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IBednetConsumer), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IBednetConsumer", "IIndividualHumanContext" );
        }
    }


/*
    Kernel::QueryResult SimpleBednet::QueryInterface( iid_t iid, void **ppinstance )
    {
        assert(ppinstance);

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(IBednet)) 
            foundInterface = static_cast<IBednet*>(this);
        // -->> add support for other I*Consumer interfaces here <<--      
        else if ( iid == GET_IID(ISupports)) 
            foundInterface = static_cast<ISupports*>(static_cast<IBednet*>(this));
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

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::SimpleBednet)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, SimpleBednet& bn, const unsigned int v)
    {
        //LOG_DEBUG("(De)serializing SimpleHousingBednet\n");

        boost::serialization::void_cast_register<SimpleBednet, IDistributableIntervention>();
        ar & bn.bednet_type;
        ar & bn.durability_time_profile;
        ar & bn.current_blockingrate;
        ar & bn.current_killingrate;
        ar & bn.primary_decay_time_constant;
        ar & bn.secondary_decay_time_constant;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(bn);
    }
}
#endif