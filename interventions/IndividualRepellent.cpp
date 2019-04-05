/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "IndividualRepellent.h"

#include "IIndividualHumanContext.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "VectorInterventionsContainerContexts.h"  // for IIndividualRepellentConsumer methods

SETUP_LOGGING( "SimpleIndividualRepellent" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(SimpleIndividualRepellent)

    bool
    SimpleIndividualRepellent::Configure(
        const Configuration * inputJson
    )
    {
        WaningConfig   blocking_config;

        initConfigComplexType("Blocking_Config", &blocking_config, SIR_Blocking_Config_DESC_TEXT );

        bool configured = BaseIntervention::Configure( inputJson );

        if( !JsonConfigurable::_dryrun  && configured )
        {
            blocking_effect = WaningEffectFactory::CreateInstance( blocking_config );
        }
        return configured;
    }

    SimpleIndividualRepellent::SimpleIndividualRepellent()
    : BaseIntervention()
    , blocking_effect(nullptr)
    , ihmc(nullptr)
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
    , blocking_effect( nullptr )
    , ihmc( nullptr )
    {
        if( master.blocking_effect != nullptr )
        {
            blocking_effect = master.blocking_effect->Clone();
        }
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
        BaseIntervention::SetContextTo( context );
        blocking_effect->SetContextTo( context );

        LOG_DEBUG("SimpleIndividualRepellent::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IIndividualRepellentConsumer), (void**)&ihmc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualRepellentConsumer", "IIndividualHumanContext" );
        }
    }

    void SimpleIndividualRepellent::Update( float dt )
    {
        //killing_effect->Update(dt);
        blocking_effect->Update(dt);
        //float current_killingrate = killing_effect->Current();
        float current_blockingrate = blocking_effect->Current();

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
    REGISTER_SERIALIZABLE(SimpleIndividualRepellent);

    void SimpleIndividualRepellent::serialize(IArchive& ar, SimpleIndividualRepellent* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleIndividualRepellent& repellent = *obj;
        ar.labelElement("blocking_effect") & repellent.blocking_effect;
//        ar.labelElement("killing_effect") & repellent.killing_effect; 
    }
}
