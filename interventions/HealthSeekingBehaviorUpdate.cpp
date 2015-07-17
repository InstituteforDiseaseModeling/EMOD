/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "HealthSeekingBehaviorUpdate.h"
#include "Contexts.h"
#include "Common.h"             // for INFINITE_TIME
#include "InterventionEnums.h"  // for InterventionDurabilityProfile, ImmunoglobulinType, etc.
#include "TBInterventionsContainer.h"  // for IHealthSeekingBehaviorEffectsUpdate


static const char * _module = "HealthSeekingBehaviorUpdate";

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
    , ihsbuea(NULL)
    {
        initSimTypes( 1, "TB_SIM" );
    }

    bool
    HealthSeekingBehaviorUpdate::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("New_Tendency", &new_probability_of_seeking, HSB_Update_New_Tendency_DESC_TEXT );
        return JsonConfigurable::Configure( inputJson );
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
        ITBInterventionsContainer * itbivc = NULL;    
        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc)  )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        return BaseIntervention::Distribute( context, pCCO );
    }

    void HealthSeekingBehaviorUpdate::Update( float dt )
    {
        ihsbuea->UpdateHealthSeekingBehaviors( new_probability_of_seeking );
        LOG_DEBUG("Update the HSB tendency\n");
        expired = true;
    }

    void HealthSeekingBehaviorUpdate::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IHealthSeekingBehaviorUpdateEffectsApply), (void**)&ihsbuea) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHealthSeekingBehaviorUpdateEffectsApply", "IIndividualHumanContext" );
        }
    }


}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::HealthSeekingBehaviorUpdate)
    namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, HealthSeekingBehaviorUpdate& bn, const unsigned int v)
    {
        static const char * _module = "HealthSeekingBehaviorUpdate";
        LOG_DEBUG("(De)serializing HealthSeekingBehaviorUpdate\n");

        ar & bn.new_probability_of_seeking;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(bn);
    }
}
#endif
