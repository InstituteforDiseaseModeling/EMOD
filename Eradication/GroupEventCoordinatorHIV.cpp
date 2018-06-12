/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <sstream>
#include "Sugar.h"
#include "Environment.h"
#include "GroupEventCoordinatorHIV.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"
#include "FactorySupport.h"
#include "InterventionFactory.h"
#include "IIndividualHuman.h"
#include "NodeDemographics.h"
#include "NodeEventContext.h"
#include "Log.h"
#include "TBContexts.h"
#include "IndividualCoInfection.h"
#include "PropertiesString.h"
#include "NodeTBHIV.h"

SETUP_LOGGING( "GroupEventCoordinatorHIV" )


namespace Kernel
{

    IMPLEMENT_FACTORY_REGISTERED(GroupInterventionDistributionEventCoordinatorHIV)

    IMPL_QUERY_INTERFACE2(GroupInterventionDistributionEventCoordinatorHIV, IEventCoordinator, IConfigurable)

    bool
    GroupInterventionDistributionEventCoordinatorHIV::Configure( const Configuration * inputJson)
    {
        initConfigTypeMap("Time_Offset", &time_offset, Time_Offset_DESC_TEXT, 0.0f, FLT_MAX, 0.0f);
        
        bool retValue = StandardInterventionDistributionEventCoordinator::Configure(inputJson);
        return retValue;
    }

    // ctor
    GroupInterventionDistributionEventCoordinatorHIV::GroupInterventionDistributionEventCoordinatorHIV()
        : StandardInterventionDistributionEventCoordinator( false )//false=don't use standard demographic coverage
        , time_offset(0.0f)
    {
        LOG_DEBUG("GroupInterventionDistributionEventCoordinatorHIV ctor\n"); 
    } 
   
   bool
    GroupInterventionDistributionEventCoordinatorHIV::visitIndividualCallback( 
        IIndividualHumanEventContext *ihec,
        float & incrementalCostOut,
        ICampaignCostObserver * pICCO
    )
   {
        bool retValue = true;

        IIndividualHumanContext* pIndHu = nullptr;

        if (const_cast<IIndividualHumanEventContext*>(ihec)->QueryInterface( GET_IID( IIndividualHumanContext ), (void**)&pIndHu ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIndividual", "IIndividualHumanCoInfection", "IIndividualHumanEventContext" );
        }

        auto node = ((IndividualHuman*)pIndHu)->GetParent();
        
        NodeTBHIV* tbhiv_node = nullptr;
        // arg, why doesn't this work!?!??
        if (node->QueryInterface( GET_IID( INodeTBHIV ), (void**)&tbhiv_node ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeTBHIV", "INodeContext" );
        }

        release_assert( tbhiv_node );

        //tbhiv_node = (NodeTBHIV*)node; // this works
        tbhiv_node = dynamic_cast<NodeTBHIV*>(node); // this works
        float dc = tbhiv_node->GetHIVCoinfectionDistribution()->DrawResultValue( ihec->GetGender() == Gender::FEMALE, (parent->GetSimulationTime().time - time_offset), ihec->GetAge() );
        LOG_DEBUG_F( "Demo coverage for individual sex %d, age %f, at time %f = %f.\n", ihec->GetGender(), ihec->GetAge(), parent->GetSimulationTime().time - time_offset, dc );
        
        demographic_restrictions.SetDemographicCoverage( dc );
        
        retValue = StandardInterventionDistributionEventCoordinator::visitIndividualCallback( ihec, incrementalCostOut, pICCO);
        
        return retValue;
    }
}
