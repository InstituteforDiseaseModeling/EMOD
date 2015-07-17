/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "STIBarrier.h"

#include <typeinfo>

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "STIInterventionsContainer.h"  // for ISTIBarrierConsumer methods
#include "IRelationship.h"

static const char* _module = "STIBarrier";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(STIBarrier)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(STIBarrier)

    IMPLEMENT_FACTORY_REGISTERED(STIBarrier)
    
    STIBarrier::STIBarrier()
    {
        LOG_DEBUG_F( "STIBarrier\n" );
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
        initConfigTypeMap( "Early", &early, STI_Barrier_Early_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "Late", &late, STI_Barrier_Late_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "MidYear", &midyear, STI_Barrier_MidYear_DESC_TEXT, 1900, 2100, 2000 );
        initConfigTypeMap( "Rate", &rate, STI_Barrier_Rate_DESC_TEXT, -100.0, 100.0, 1.0 );
    }

    bool
    STIBarrier::Configure(
        const Configuration * inputJson
    )
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        //initConfig( "Durability_Time_Profile", durability_time_profile, inputJson, MetadataDescriptor::Enum("Durability_Time_Profile", SB_Durability_Time_Profile_DESC_TEXT, MDD_ENUM_ARGS(InterventionDurabilityProfile)) );
        initConfig( "Relationship_Type", rel_type, inputJson, MetadataDescriptor::Enum("Relationship_Type", STI_Barrier_Relationship_Type_DESC_TEXT, MDD_ENUM_ARGS(RelationshipType)) );
        return JsonConfigurable::Configure( inputJson );
    }

    bool
    STIBarrier::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(ISTIBarrierConsumer), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTIBarrierConsumer", "IIndividualHumanInterventionsContext" );
        }
        //context->PurgeExisting( typeid(*this).name() );
        return BaseIntervention::Distribute( context, pCCO );
    }

    void STIBarrier::Update( float dt )
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        SigmoidConfig probs;
        probs.early = early;
        probs.late = late;
        probs.midyear = midyear;
        probs.rate = rate;
        ibc->UpdateSTIBarrierProbabilitiesByType( rel_type, probs );
        expired = true;
    }

    void STIBarrier::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(ISTIBarrierConsumer), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTIBarrierConsumer", "IIndividualHumanContext" );
        }
    }


/*
    Kernel::QueryResult STIBarrier::QueryInterface( iid_t iid, void **ppinstance )
    {
        assert(ppinstance);

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(ISTIBarrier)) 
            foundInterface = static_cast<ISTIBarrier*>(this);
        // -->> add support for other I*Consumer interfaces here <<--      
        else if ( iid == GET_IID(ISupports)) 
            foundInterface = static_cast<ISupports*>(static_cast<ISTIBarrier*>(this));
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
BOOST_CLASS_EXPORT(Kernel::STIBarrier)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, STIBarrier& bn, const unsigned int v)
    {
        //LOG_DEBUG("(De)serializing SimpleHousingSTIBarrier\n");

        boost::serialization::void_cast_register<STIBarrier, IDistributableIntervention>();
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(bn);
    }
}
#endif
