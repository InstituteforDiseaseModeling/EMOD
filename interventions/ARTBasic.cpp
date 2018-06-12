/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
    BEGIN_QUERY_INTERFACE_BODY( ARTBasic )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( ARTBasic )

    IMPLEMENT_FACTORY_REGISTERED(ARTBasic)

    ARTBasic::ARTBasic()
    : BaseIntervention()
    , viral_suppression(true)
    , days_to_achieve_suppression(183.0f)
    {
        initSimTypes( 2, "HIV_SIM", "TBHIV_SIM" );
    }

    ARTBasic::~ARTBasic()
    {
    }

    bool
    ARTBasic::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Cost_To_Consumer", &cost_per_unit, DRUG_Cost_To_Consumer_DESC_TEXT, 0, 99999);
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
        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            LOG_DEBUG_F( "ARTBasic distributed to individual %d.\n", context->GetParent()->GetSuid().data );
            IHIVDrugEffectsApply* itbda = nullptr;
            if (s_OK != context->QueryInterface(GET_IID(IHIVDrugEffectsApply), (void**)&itbda) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVDrugEffectsApply", "IIndividualHumanInterventionsContext" );
            } 
            itbda->GoOnART( viral_suppression, days_to_achieve_suppression );
        }
        return distributed;
    }

    void ARTBasic::Update( float dt )
    {
        Expired();
    }

    REGISTER_SERIALIZABLE(ARTBasic);

    void ARTBasic::serialize(IArchive& ar, ARTBasic* obj)
    {
        BaseIntervention::serialize( ar, obj );
        ARTBasic& art = *obj;
        ar.labelElement("viral_suppression"          ) & art.viral_suppression;
        ar.labelElement("days_to_achieve_suppression") & art.days_to_achieve_suppression;
    }
}
