/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVPreARTNotification.h"

#include "Contexts.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"

#include "STIInterventionsContainer.h"

#include "IHIVInterventionsContainer.h" // for time-date util function and access into IHIVCascadeOfCare
#include "IIndividualHumanHIV.h"

static const char * _module = "HIVPreARTNotification";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HIVPreARTNotification)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(HIVPreARTNotification)

    IMPLEMENT_FACTORY_REGISTERED(HIVPreARTNotification)
    
    HIVPreARTNotification::HIVPreARTNotification() :
        startingPreART(0)
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    bool
    HIVPreARTNotification::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Starting_PreART", &startingPreART, HIV_Starting_PreART_DESC_TEXT , true);

        return JsonConfigurable::Configure( inputJson );
    }

    bool HIVPreARTNotification::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        IIndividualHumanContext *parent = context->GetParent();
        LOG_DEBUG_F( "Individual %d is getting tested.\n", parent->GetSuid().data );

        IIndividualHumanHIV * hiv_parent = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanContext" );
        }
        IHIVMedicalHistory * med_parent = nullptr;
        if (parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVMedicalHistory), (void**)&med_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVMedicalHistory", "IIndividualHumanContext" );
        }

        if( startingPreART ) {
            med_parent->OnBeginPreART();
        } else {
            med_parent->OnEndPreART();
        }

        return BaseIntervention::Distribute( context, pICCO );
    }

    void HIVPreARTNotification::Update( float dt )
    {
        // Nothing to do for this intervention, which doesn't have ongoing effects
        expired = true;
    }

    REGISTER_SERIALIZABLE(HIVPreARTNotification);

    void HIVPreARTNotification::serialize(IArchive& ar, HIVPreARTNotification* obj)
    {
        BaseIntervention::serialize( ar, obj );
        HIVPreARTNotification& note = *obj;

        ar.labelElement("startingPreART") & note.startingPreART;
    }
}
