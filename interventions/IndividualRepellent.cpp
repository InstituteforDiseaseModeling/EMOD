/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "IndividualRepellent.h"

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "VectorInterventionsContainer.h"  // for IIndividualRepellentConsumer methods

static const char* _module = "SimpleIndividualRepellent";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(SimpleIndividualRepellent)

    bool
    SimpleIndividualRepellent::Configure(
        const Configuration * inputJson
    )
    {
        initConfig( "Durability_Time_Profile", durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Time_Profile", SIR_Durability_Time_Profile_DESC_TEXT, MDD_ENUM_ARGS(InterventionDurabilityProfile)) );
        return JsonConfigurable::Configure( inputJson );
    }

    SimpleIndividualRepellent::SimpleIndividualRepellent()
        : durability_time_profile( InterventionDurabilityProfile::BOXDURABILITY )
        , current_blockingrate(0.0f)
        , current_killingrate(0.0f)
        , primary_decay_time_constant(0.0f)
        , secondary_decay_time_constant(0.0f)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap("Primary_Decay_Time_Constant", &primary_decay_time_constant, SIR_Primary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 3650);
        initConfigTypeMap("Secondary_Decay_Time_Constant", &secondary_decay_time_constant, SIR_Secondary_Decay_Time_Constant_DESC_TEXT, 0, 1000000, 3650);
        initConfigTypeMap("Blocking_Rate", &current_blockingrate, SIR_Blocking_Rate_DESC_TEXT, 0.0, 1.0, 1.0);
        //initConfigTypeMap("Killing_Rate", &current_killingrate, SIR_Killing_Rate_DESC_TEXT, 0.0, 1.0, 0.0); // Force individual repellent to have no configurable killing here (EAW)
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, SIR_Cost_To_Consumer_DESC_TEXT, 0, 999999, 8.0);
    }

    bool
    SimpleIndividualRepellent::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IIndividualRepellentConsumer), (void**)&ihmc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualRepellentConsumer", "IIndividualHumanInterventionsContext" );
        }
        return BaseIntervention::Distribute( context, pCCO );
    }

    void SimpleIndividualRepellent::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        LOG_DEBUG("SimpleIndividualRepellent::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IIndividualRepellentConsumer), (void**)&ihmc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualRepellentConsumer", "IIndividualHumanContext" );
        }
    }

    void SimpleIndividualRepellent::Update( float dt )
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
            ihmc->UpdateProbabilityOfIndRepBlocking( current_blockingrate );
            ihmc->UpdateProbabilityOfIndRepKilling( current_killingrate );
        }
        else
        {
            // ERROR: ihmc (interventions container) pointer null. Should be impossible to get here, but that's 
            // what one always says about null pointers! :)
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "ihmc", "IIndividualRepellentConsumer" );
        }
    }

    BEGIN_QUERY_INTERFACE_BODY(SimpleIndividualRepellent)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(SimpleIndividualRepellent)
/*
    Kernel::QueryResult SimpleIndividualRepellent::QueryInterface( iid_t iid, void **ppinstance )
    {
        assert(ppinstance);

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(IIndividualRepellent)) 
            foundInterface = static_cast<IIndividualRepellent*>(this);
        // -->> add support for other I*Consumer interfaces here <<--      
      
        else if ( iid == GET_IID(ISupports)) 
            foundInterface = static_cast<ISupports*>(static_cast<IIndividualRepellent*>(this));
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
BOOST_CLASS_EXPORT(Kernel::SimpleIndividualRepellent)

namespace Kernel {

    template<class Archive>
    void serialize(Archive &ar, SimpleIndividualRepellent& obj, const unsigned int v)
    {
        static const char * _module = "SimpleIndividualRepellent";
        LOG_DEBUG("(De)serializing SimpleIndividualRepellent\n");

        boost::serialization::void_cast_register<SimpleIndividualRepellent, IDistributableIntervention>();
        ar & obj.durability_time_profile;
        ar & obj.current_blockingrate;
        ar & obj.current_killingrate;
        ar & obj.primary_decay_time_constant;
        ar & obj.secondary_decay_time_constant;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(obj);
    }
}
#endif
