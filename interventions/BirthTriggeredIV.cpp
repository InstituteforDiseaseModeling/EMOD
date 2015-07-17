/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "BirthTriggeredIV.h"

#include "InterventionFactory.h"
#include "Log.h"
#include "Debug.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)

static const char * _module = "BirthTriggeredIV";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(BirthTriggeredIV)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_INTERFACE(IIndividualEventObserver)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(BirthTriggeredIV)

    IMPLEMENT_FACTORY_REGISTERED(BirthTriggeredIV)

    BirthTriggeredIV::BirthTriggeredIV() : 
    parent(NULL) 
    , duration(0)
    , max_duration(0)
    , demographic_coverage(1.0)
    {
    }

    BirthTriggeredIV::~BirthTriggeredIV() { }
    int BirthTriggeredIV::AddRef() { return BaseNodeIntervention::AddRef(); }
    int BirthTriggeredIV::Release() { return BaseNodeIntervention::Release(); }

    bool
    BirthTriggeredIV::Configure(
        const Configuration * inputJson
    )
    {
        initConfigComplexType("Actual_IndividualIntervention_Config", &actual_intervention_config, BT_Actual_Intervention_Config_DESC_TEXT);
        initConfigTypeMap("Duration", &max_duration, BT_Duration_DESC_TEXT, -1.0f, FLT_MAX, -1.0f ); // -1 is a convention for indefinite duration
        initConfigTypeMap("Demographic_Coverage", &demographic_coverage, BT_Demographic_Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret )
        {
            InterventionValidator::ValidateIntervention( actual_intervention_config._json );
        }
        return ret ;
    }

    bool
    BirthTriggeredIV::Distribute(
        INodeEventContext *pNodeEventContext,
        IEventCoordinator2 *pEC
    )
    {
        LOG_DEBUG_F("Distributed birth-triggered intervention to NODE: %d\n", pNodeEventContext->GetId().data);

        // QI to register ourself as a birth observer
        INodeTriggeredInterventionConsumer * pNTIC = NULL;
        if (s_OK != pNodeEventContext->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNodeEventContext", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
        }
        release_assert( pNTIC );
        pNTIC->RegisterNodeEventObserver( this, IndividualEventTriggerType::Births );

        // We can QI NEC to get the campaign cost observer (it's just the node!)
        ICampaignCostObserver *iCCO;
        if (s_OK != parent->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&iCCO))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ICampaignCostObserver", "INodeEventContext" );
        }
        assert( iCCO );
        return true;
    }

    bool BirthTriggeredIV::notifyOnEvent(
        IIndividualHumanEventContext *pIndiv,
        const std::string& StateChange
    )
    {
        LOG_DEBUG("A baby was born, distribute actual_intervention (conditionally)\n");

        assert( parent );
        assert( parent->GetRng() );
        double randomDraw = parent->GetRng()->e();

        // want some way to demonstrate selective distribution of calender; no rng available to us, individual property value???
        LOG_DEBUG_F("demographic_coverage = %f\n", demographic_coverage);
        if( randomDraw > demographic_coverage )
        {
            LOG_DEBUG("Demographic coverage ruled this out\n");
            return false;
        }

        // Query for campaign cost observer interface from INodeEventContext *parent
        ICampaignCostObserver *iCCO;
        if (s_OK != parent->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&iCCO))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ICampaignCostObserver", "INodeEventContext" );
        }

        // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
        //const IInterventionFactory* ifobj = dynamic_cast<NodeEventContextHost *>(parent)->GetInterventionFactoryObj();
        IGlobalContext *pGC = NULL;
        const IInterventionFactory* ifobj = NULL;
        if (s_OK == parent->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
        {
            ifobj = pGC->GetInterventionFactory();
        }
        if (!ifobj)
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
        }

        IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention(Configuration::CopyFromElement( actual_intervention_config._json ) ); 
        if( di )
        {
            di->Distribute( pIndiv->GetInterventionsContext(), iCCO );

            LOG_DEBUG("A birth-triggered intervention was successfully distributed\n");
            // It's not at all clear to me that we would incur cost at this point, but we could.
            //iCCO->notifyCampaignExpenseIncurred( interventionCost, pIndiv );
            return true;
        }

        return false;
    }

    void BirthTriggeredIV::Update( float dt )
    {
        duration += dt;
        if( max_duration >= 0 && duration > max_duration )
        {
            // QI to register ourself as a birth observer
            INodeTriggeredInterventionConsumer * pNTIC = nullptr;
            if (s_OK == parent->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&pNTIC) )
            {
                release_assert( pNTIC );
                pNTIC->UnregisterNodeEventObserver( this, IndividualEventTriggerType::Births );
            }
            else
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualTriggeredInterventionConsumer", "INodeEventContext" );
            }
        }
    }

    void BirthTriggeredIV::SetContextTo(INodeEventContext *context) 
    { 
        parent = context; 
    }

#if USE_JSON_SERIALIZATION
    // It is double inheritance from both BaseIntervention and IIndividualEventObserver
    // For JSON serialization
    void BirthTriggeredIV::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("actual_intervention_config");
        actual_intervention_config.JSerialize(root, helper);
        root->Insert("demographic_coverage", demographic_coverage);
        root->Insert("max_duration", max_duration);

        root->EndObject();
    }

    void BirthTriggeredIV::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
    }
#endif
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::BirthTriggeredIV)

namespace Kernel {
    REGISTER_SERIALIZATION_VOID_CAST(BirthTriggeredIV, INodeDistributableIntervention)
    REGISTER_SERIALIZATION_VOID_CAST(BirthTriggeredIV, IIndividualEventObserver)
    template<class Archive>
    void serialize(Archive &ar, BirthTriggeredIV& iv, const unsigned int v)
    {
        ar & iv.actual_intervention_config;
        ar & iv.demographic_coverage;
        ar & iv.efficacy;
        ar & iv.max_duration;
    }
}
#endif
