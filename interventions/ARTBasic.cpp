/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

//#ifdef ENABLE_STI

#include "ARTBasic.h"

#include "Contexts.h"                  // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "Debug.h"                  // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "IHIVInterventionsContainer.h"  // for IHIVDrugEffectsApply methods

SETUP_LOGGING( "ARTBasic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(ARTBasic, GenericDrug)
    END_QUERY_INTERFACE_DERIVED(ARTBasic, GenericDrug)

    IMPLEMENT_FACTORY_REGISTERED(ARTBasic)

    ARTBasic::ARTBasic()
    : GenericDrug()
    , itbda(nullptr)
    , viral_suppression(true)
    , days_to_achieve_suppression(183.0f)
    {
        initSimTypes( 2, "HIV_SIM", "TBHIV_SIM" );
    }

    ARTBasic::~ARTBasic()
    {
    }

    std::string
    ARTBasic::GetDrugName() const
    {
        return std::string("ART");
    }

    bool
    ARTBasic::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap ( "Cost_To_Consumer", &cost_per_unit, DRUG_Cost_To_Consumer_DESC_TEXT, 0, 99999);
        initConfigTypeMap( "Viral_Suppression", &viral_suppression, ART_Basic_Viral_Suppression_DESC_TEXT, true );
        initConfigTypeMap( "Days_To_Achieve_Viral_Suppression", &days_to_achieve_suppression, ART_Basic_Days_To_Achieve_Viral_Suppression_DESC_TEXT, 0, FLT_MAX, 183.0f );

        // Skip GenericDrug (base class) Configure (to avoid picking up all those parameters). Connection with GenericDrug is fairly loose.
        return BaseIntervention::Configure( inputJson );
    }

    bool
    ARTBasic::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool distributed = GenericDrug::Distribute( context, pCCO );
        if( distributed )
        {
            LOG_DEBUG_F( "ARTBasic distributed to individual %d.\n", context->GetParent()->GetSuid().data );
            if (s_OK != context->QueryInterface(GET_IID(IHIVDrugEffectsApply), (void**)&itbda) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVDrugEffectsApply", "IIndividualHumanInterventionsContext" );
            } 
            itbda->GoOnART( viral_suppression, days_to_achieve_suppression );
        }
        return distributed;
    }

    void
    ARTBasic::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHIVDrugEffectsApply), (void**)&itbda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVDrugEffectsApply", "IIndividualHumanContext" );
        } 

        return GenericDrug::SetContextTo( context );
    }

    void ARTBasic::ApplyEffects()
    {
        assert(itbda);
        //itbda->ApplyDrugVaccineReducedAcquireEffect(GetDrugReducedAcquire());
        //itbda->ApplyDrugVaccineReducedTransmitEffect(GetDrugReducedTransmit());
        //itbda->ApplyDrugInactivationRateEffect( GetDrugInactivationRate() );
        //itbda->ApplyDrugClearanceRateEffect( GetDrugClearanceRate() );
    }

    REGISTER_SERIALIZABLE(ARTBasic);

    void ARTBasic::serialize(IArchive& ar, ARTBasic* obj)
    {
        GenericDrug::serialize( ar, obj );
        ARTBasic& art = *obj;
        ar.labelElement("viral_suppression"          ) & art.viral_suppression;
        ar.labelElement("days_to_achieve_suppression") & art.days_to_achieve_suppression;

        // itbda set in SetContextTo
    }
}

//#endif // ENABLE_STI
