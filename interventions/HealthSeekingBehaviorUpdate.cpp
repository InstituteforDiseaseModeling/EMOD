/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HealthSeekingBehaviorUpdate.h"
#include "Contexts.h"
#include "Common.h"             // for INFINITE_TIME
#include "TBInterventionsContainer.h"  // for IHealthSeekingBehaviorEffectsUpdate


SETUP_LOGGING( "HealthSeekingBehaviorUpdate" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(HealthSeekingBehaviorUpdate)

    BEGIN_QUERY_INTERFACE_BODY(HealthSeekingBehaviorUpdate)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(HealthSeekingBehaviorUpdate)

    HealthSeekingBehaviorUpdate::HealthSeekingBehaviorUpdate()
    :new_probability_of_seeking(0.0f)
    , ihsbuea(nullptr)
    {
        initSimTypes( 0 );
    }

    bool
    HealthSeekingBehaviorUpdate::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("New_Tendency", &new_probability_of_seeking, HSB_Update_New_Tendency_DESC_TEXT );
        return BaseIntervention::Configure( inputJson );
    }

    bool
    HealthSeekingBehaviorUpdate::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pCCO
    )
    {
        if (s_OK != context->QueryInterface(GET_IID(IHealthSeekingBehaviorUpdateEffectsApply), (void**)&ihsbuea) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHealthSeekingBehaviorUpdateEffectsApply", "IIndividualHumanInterventionsContext" );
        }

        // this intervention only works with TB Interventions Container, throw an error if you are not using that
        ITBInterventionsContainer * itbivc = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc)  )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        return BaseIntervention::Distribute( context, pCCO );
    }

    void HealthSeekingBehaviorUpdate::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        ihsbuea->UpdateHealthSeekingBehaviors( new_probability_of_seeking );
        LOG_DEBUG_F( "Update the HSB tendency with value %f\n", new_probability_of_seeking );
        expired = true;
    }

    void HealthSeekingBehaviorUpdate::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        BaseIntervention::SetContextTo( context );
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHealthSeekingBehaviorUpdateEffectsApply), (void**)&ihsbuea) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHealthSeekingBehaviorUpdateEffectsApply", "IIndividualHumanContext" );
        }
    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, HealthSeekingBehaviorUpdate& bn, const unsigned int v)
    {
        ar & bn.new_probability_of_seeking;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(bn);
    }
}
#endif
