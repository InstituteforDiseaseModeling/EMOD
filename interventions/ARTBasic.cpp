/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

//#ifdef ENABLE_STI

#include "ARTBasic.h"

#include "Contexts.h"                  // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "Debug.h"                  // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "HIVInterventionsContainer.h"  // for IHIVDrugEffectsApply methods

static const char* _module = "ARTBasic";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(ARTBasic, GenericDrug)
    END_QUERY_INTERFACE_DERIVED(ARTBasic, GenericDrug)

    IMPLEMENT_FACTORY_REGISTERED(ARTBasic)

    ARTBasic::ARTBasic()
    : GenericDrug()
    , itbda(NULL)
    , viral_suppression(true)
    , days_to_achieve_suppression(183.0f)
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    ARTBasic::~ARTBasic()
    {
    }

    std::string
    ARTBasic::GetDrugName()
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
        return JsonConfigurable::Configure( inputJson );
    }

    bool
    ARTBasic::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        LOG_DEBUG_F( "ARTBasic distributed to individual %d.\n", context->GetParent()->GetSuid().data );
        if (s_OK != context->QueryInterface(GET_IID(IHIVDrugEffectsApply), (void**)&itbda) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVDrugEffectsApply", "IIndividualHumanInterventionsContext" );
        } 
        itbda->GoOnART( viral_suppression, days_to_achieve_suppression );

        return GenericDrug::Distribute( context, pCCO );
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
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::ARTBasic)
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, ARTBasic& drug, const unsigned int v)
    {
        boost::serialization::void_cast_register<ARTBasic, IDrug>();
        //ar & drug.drug_type;
        ar & boost::serialization::base_object<GenericDrug>(drug);
    }
}
#endif

//#endif // ENABLE_STI
