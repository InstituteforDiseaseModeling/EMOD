/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "STIBarrier.h"

#include <typeinfo>

#include "Contexts.h"                      // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "STIInterventionsContainer.h"  // for ISTIBarrierConsumer methods
#include "IRelationship.h"
#include "Sigmoid.h"

SETUP_LOGGING( "STIBarrier" )

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
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, STI_Barrier_Cost_DESC_TEXT, 0, 999999, 10.0);
        initConfigTypeMap( "Early", &early, STI_Barrier_Early_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "Late", &late, STI_Barrier_Late_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "MidYear", &midyear, STI_Barrier_MidYear_DESC_TEXT, MIN_YEAR, MAX_YEAR, 2000 );
        initConfigTypeMap( "Rate", &rate, STI_Barrier_Rate_DESC_TEXT, -100.0, 100.0, 1.0 );
    }

    bool
    STIBarrier::Configure(
        const Configuration * inputJson
    )
    {
        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        initConfig( "Relationship_Type", rel_type, inputJson, MetadataDescriptor::Enum("Relationship_Type", STI_Barrier_Relationship_Type_DESC_TEXT, MDD_ENUM_ARGS(RelationshipType)) );
        return BaseIntervention::Configure( inputJson );
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
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        LOG_DEBUG_F( "%s\n", __FUNCTION__ );
        Sigmoid probs( early, late, midyear, rate );
        ibc->UpdateSTIBarrierProbabilitiesByType( rel_type, probs );
        expired = true;
    }

    void STIBarrier::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        BaseIntervention::SetContextTo( context );

        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(ISTIBarrierConsumer), (void**)&ibc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTIBarrierConsumer", "IIndividualHumanContext" );
        }
    }

    REGISTER_SERIALIZABLE(STIBarrier);

    void STIBarrier::serialize(IArchive& ar, STIBarrier* obj)
    {
        BaseIntervention::serialize( ar, obj );
        STIBarrier& barrier = *obj;
        ar.labelElement("early"   ) & barrier.early;
        ar.labelElement("late"    ) & barrier.late;
        ar.labelElement("midyear" ) & barrier.midyear;
        ar.labelElement("rate"    ) & barrier.rate;
        ar.labelElement("rel_type") & (uint32_t&)barrier.rel_type;

        // ibc is set in SetContextTo()
    }
}
