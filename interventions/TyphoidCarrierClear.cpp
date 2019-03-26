/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "TyphoidCarrierClear.h"
#include "TyphoidInterventionsContainer.h"
#include "IndividualTyphoid.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "TBContexts.h"

SETUP_LOGGING( "TyphoidCarrierClear" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(TyphoidCarrierClear)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(TyphoidCarrierClear)
    
    IMPLEMENT_FACTORY_REGISTERED(TyphoidCarrierClear)

#define TYPHOID_CARRIER_CLEARANCE_RATE_DESC_TEXT ""
    bool TyphoidCarrierClear::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Clearance_Rate", &clearance_rate, TYPHOID_CARRIER_CLEARANCE_RATE_DESC_TEXT, 0, 1.0, 1.0 );
        return JsonConfigurable::Configure( inputJson );
    }

    TyphoidCarrierClear::TyphoidCarrierClear() : BaseIntervention()
    {
        initSimTypes( 1, "TYPHOID_SIM" );
    }

    TyphoidCarrierClear::~TyphoidCarrierClear()
    { 
        LOG_DEBUG("Destructing TyphoidCarrier Clear\n");
    }

    bool TyphoidCarrierClear::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool distributed = false;
        // TBD: Get individual from context, and infect
        //auto individual = dynamic_cast<IIndividualHumanTyphoid*>(context->GetParent()); // QI in new code
        //INodeEventContext * pContext = individual->GetParent()->GetEventContext();
        LOG_DEBUG_F( "Setting clearance rate of %f on individual.\n", clearance_rate  );
        if( dynamic_cast<IndividualHumanTyphoid*>(context->GetParent())->IsChronicCarrier( false ) )
        {
            dynamic_cast<TyphoidInterventionsContainer*>(context)->ApplyClearance( clearance_rate );
        }

        distributed = true;
        
        return distributed;
    }

    void TyphoidCarrierClear::Update( float dt )
    {
        // Distribute() doesn't call GiveIntervention() for this intervention, so it isn't added to the NodeEventContext's list of NDI
    }

    //REGISTER_SERIALIZABLE(TyphoidCarrierClear);
}


