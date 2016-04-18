/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

//#ifdef ENABLE_STI

#include "ARTDropout.h"

#include "Contexts.h"                  // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "Debug.h"                  // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "IHIVInterventionsContainer.h"  // for IHIVDrugEffectsApply methods

static const char* _module = "ARTDropout";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(ARTDropout, GenericDrug)
    END_QUERY_INTERFACE_DERIVED(ARTDropout, GenericDrug)

    IMPLEMENT_FACTORY_REGISTERED(ARTDropout)

    ARTDropout::ARTDropout()
    : GenericDrug()
    , itbda(nullptr)
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    ARTDropout::~ARTDropout()
    {
    }

    std::string
    ARTDropout::GetDrugName()
    {
        return std::string("ART");
    }

    bool
    ARTDropout::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, DRUG_Cost_To_Consumer_DESC_TEXT, 0, 99999);
        // Skip GenericDrug (base class) Configure (to avoid picking up all those parameters). Connection with GenericDrug is fairly loose.
        return JsonConfigurable::Configure( inputJson );
    }

    bool
    ARTDropout::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        LOG_DEBUG_F( "ARTDropout distributed to individual %d.\n", context->GetParent()->GetSuid().data );
        if (s_OK != context->QueryInterface(GET_IID(IHIVDrugEffectsApply), (void**)&itbda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVDrugEffectsApply", "IIndividualHumanInterventionsContext" );
        } 
        itbda->GoOffART();

        return GenericDrug::Distribute( context, pCCO );
    }

    void
    ARTDropout::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHIVDrugEffectsApply), (void**)&itbda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVDrugEffectsApply", "IIndividualHumanContext" );
        } 

        return GenericDrug::SetContextTo( context );
    }

    void ARTDropout::ApplyEffects()
    {
        assert(itbda);
        //itbda->ApplyDrugVaccineReducedAcquireEffect(GetDrugReducedAcquire());
        //itbda->ApplyDrugVaccineReducedTransmitEffect(GetDrugReducedTransmit());
        //itbda->ApplyDrugInactivationRateEffect( GetDrugInactivationRate() );
        //itbda->ApplyDrugClearanceRateEffect( GetDrugClearanceRate() );
    }

    REGISTER_SERIALIZABLE(ARTDropout);

    void ARTDropout::serialize(IArchive& ar, ARTDropout* obj)
    {
        GenericDrug::serialize( ar, obj );
        ARTDropout& art = *obj;

        // itbda set in SetContextTo
    }
}

//#endif // ENABLE_STI
