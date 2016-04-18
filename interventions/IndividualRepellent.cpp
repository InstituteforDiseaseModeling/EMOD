/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

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
        initConfigComplexType("Blocking_Config", &blocking_config, SIR_Blocking_Config_DESC_TEXT );
        bool configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_blocking = Configuration::CopyFromElement( blocking_config._json );
            blocking_effect = WaningEffectFactory::CreateInstance( tmp_blocking );
            delete tmp_blocking;
            tmp_blocking = nullptr;
        }
        return configured;
    }

    SimpleIndividualRepellent::SimpleIndividualRepellent()
        : blocking_effect( nullptr )
        , current_blockingrate( 0.0f )
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, SIR_Cost_To_Consumer_DESC_TEXT, 0, 999999, 8.0);
    }

    SimpleIndividualRepellent::~SimpleIndividualRepellent()
    {
        delete blocking_effect;
    }

    SimpleIndividualRepellent::SimpleIndividualRepellent( const SimpleIndividualRepellent& master )
    : BaseIntervention( master )
    {
        blocking_config = master.blocking_config;
        auto tmp_blocking = Configuration::CopyFromElement( blocking_config._json );
        blocking_effect = WaningEffectFactory::CreateInstance( tmp_blocking );
        delete tmp_blocking;
        tmp_blocking = nullptr;
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
        //current_killingrate = killing_effect->Current();
        current_blockingrate = blocking_effect->Current();
    }

    void SimpleIndividualRepellent::Update( float dt )
    {
        //killing_effect->Update(dt);
        blocking_effect->Update(dt);
        //current_killingrate = killing_effect->Current();
        current_blockingrate = blocking_effect->Current();

        if( ihmc )
        {
            ihmc->UpdateProbabilityOfIndRepBlocking( current_blockingrate );
            //ihmc->UpdateProbabilityOfIndRepKilling( current_killingrate );
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

    REGISTER_SERIALIZABLE(SimpleIndividualRepellent);

    void SimpleIndividualRepellent::serialize(IArchive& ar, SimpleIndividualRepellent* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleIndividualRepellent& repellent = *obj;
        ar.labelElement("current_blockingrate") & repellent.current_blockingrate;
        ar.labelElement("current_killingrate") & repellent.current_killingrate; 
        ar.labelElement("blocking_effect") & repellent.blocking_effect;
//        ar.labelElement("killing_effect") & repellent.killing_effect; 
    }
}
