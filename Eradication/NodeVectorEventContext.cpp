/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "stdafx.h"
#include "NodeVectorEventContext.h"

#include "NodeVector.h"
#include "SimulationEventContext.h"
#include "Debug.h"

SETUP_LOGGING( "NodeVectorEventContext" )

namespace Kernel
{
    NodeVectorEventContextHost::NodeVectorEventContextHost(Node* _node) 
    : NodeEventContextHost(_node)
    , larval_reduction( false, -FLT_MAX, FLT_MAX, 0.0f )
    , pLarvalKilling(0)
    , pLarvalHabitatReduction(0)
    , pVillageSpatialRepellent(0)
    , pADIVAttraction(0)
    , pADOVAttraction(0)
    , pPFVKill(0)
    , pOutdoorKilling(0)
    , pOutdoorKillingMale(0)
    , pSugarFeedKilling(0)
    , pOviTrapKilling(0)
    , pAnimalFeedKilling(0)
    , pOutdoorRestKilling(0)
    , pIndoorKilling( 0.0f )
    { 
    }

    NodeVectorEventContextHost::~NodeVectorEventContextHost()
    {
    }

    QueryResult NodeVectorEventContextHost::QueryInterface( iid_t iid, void** ppinstance )
    {
        release_assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(INodeVectorInterventionEffects)) 
            foundInterface = static_cast<INodeVectorInterventionEffects*>(this);
        else if (iid == GET_IID(INodeVectorInterventionEffectsApply))
            foundInterface = static_cast<INodeVectorInterventionEffectsApply*>(this);
        else if (iid == GET_IID(IMosquitoReleaseConsumer))
            foundInterface = static_cast<IMosquitoReleaseConsumer*>(this);

        // -->> add support for other I*Consumer interfaces here <<--      
        else
            foundInterface = nullptr;

        QueryResult status; // = e_NOINTERFACE;
        if ( !foundInterface )
        {
            //status = e_NOINTERFACE;
            status = NodeEventContextHost::QueryInterface(iid, (void**)&foundInterface);
        }
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    //
    // INodeVectorInterventionEffects; (The Getters)
    // 
    void
    NodeVectorEventContextHost::UpdateLarvalKilling(
        VectorHabitatType::Enum habitat,
        float killing
    )
    {
        larval_killing_target = habitat;
        pLarvalKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateLarvalHabitatReduction(
        VectorHabitatType::Enum target,
        float reduction
    )
    {
        larval_reduction_target = target;
        larval_reduction.Initialize();
        larval_reduction.SetMultiplier( target, reduction );
    }

    void
    NodeVectorEventContextHost::UpdateLarvalHabitatReduction( const LarvalHabitatMultiplier& lhm )
    {
        larval_reduction.Initialize();
        larval_reduction.SetAsReduction( lhm );
    }

    void
    NodeVectorEventContextHost::UpdateOutdoorKilling(
        float killing
    )
    {
        pOutdoorKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateVillageSpatialRepellent(
        float reduction)
    {
        pVillageSpatialRepellent = reduction;
    }

    void
    NodeVectorEventContextHost::UpdateADIVAttraction(
        float reduction
    )
    {
        pADIVAttraction = reduction;
    }

    void
    NodeVectorEventContextHost::UpdateADOVAttraction(
        float reduction
    )
    {
        pADOVAttraction = reduction;
    }

    void
    NodeVectorEventContextHost::UpdatePFVKill(
        float killing
    )
    {
        pPFVKill = killing;
    }

    void
    NodeVectorEventContextHost::UpdateOutdoorKillingMale(
        float killing
    )
    {
        pOutdoorKillingMale = killing;
    }

    void
    NodeVectorEventContextHost::UpdateSugarFeedKilling(
        float killing
    )
    {
        pSugarFeedKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateOviTrapKilling(
        VectorHabitatType::Enum habitat,
        float killing
    )
    {
        ovitrap_killing_target = habitat;
        pOviTrapKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateAnimalFeedKilling(
        float killing
    )
    {
        pAnimalFeedKilling = killing;
    }

    void
    NodeVectorEventContextHost::UpdateOutdoorRestKilling(
        float killing
    )
    {
        pOutdoorRestKilling = killing;
    }

    void NodeVectorEventContextHost::UpdateIndoorKilling( float killing )
    {
        pIndoorKilling = killing;
    }

    //
    // INodeVectorInterventionEffects; (The Getters)
    // 
    float NodeVectorEventContextHost::GetLarvalKilling(
        VectorHabitatType::Enum habitat_query 
    )
    {
        if( larval_killing_target == VectorHabitatType::ALL_HABITATS ||
            habitat_query == larval_killing_target )
        {
            return pLarvalKilling;
        }
        else
        {
            return 0;
        }
    }

    float NodeVectorEventContextHost::GetLarvalHabitatReduction(
        VectorHabitatType::Enum habitat_query, // shouldn't the type be the enum???
        const std::string& species
    )
    {
        VectorHabitatType::Enum vht = habitat_query;
        if( larval_reduction_target == VectorHabitatType::ALL_HABITATS )
        {
            vht = VectorHabitatType::ALL_HABITATS;
        }
        if( !larval_reduction.WasInitialized() )
        {
            larval_reduction.Initialize();
        }
        float ret = larval_reduction.GetMultiplier( vht, species );

        LOG_DEBUG_F( "%s returning %f (habitat_query = %s)\n", __FUNCTION__, ret, VectorHabitatType::pairs::lookup_key( habitat_query ) );
        return ret;
    }

    float NodeVectorEventContextHost::GetVillageSpatialRepellent()
    {
        return pVillageSpatialRepellent;
    }

    float NodeVectorEventContextHost::GetADIVAttraction()
    {
        return pADIVAttraction;
    }

    float NodeVectorEventContextHost::GetADOVAttraction()
    {
        return pADOVAttraction;
    }

    float NodeVectorEventContextHost::GetPFVKill()
    {
        return pPFVKill;
    }

    float NodeVectorEventContextHost::GetOutdoorKilling()
    {
        return pOutdoorKilling;
    }

    float NodeVectorEventContextHost::GetOutdoorKillingMale()
    {
        return pOutdoorKillingMale;
    }

    float NodeVectorEventContextHost::GetSugarFeedKilling()
    {
        return pSugarFeedKilling;
    }

    float NodeVectorEventContextHost::GetOviTrapKilling(
        VectorHabitatType::Enum habitat_query
    )
    {
        if( ovitrap_killing_target == VectorHabitatType::ALL_HABITATS ||
            habitat_query == ovitrap_killing_target )
        {
            return pOviTrapKilling; 
        }
        else
        {
            return 0;
        }
    }
    float NodeVectorEventContextHost::GetAnimalFeedKilling()
    {
        return pAnimalFeedKilling;
    }
    float NodeVectorEventContextHost::GetOutdoorRestKilling()
    {
        return pOutdoorRestKilling;
    }

    float NodeVectorEventContextHost::GetIndoorKilling()
    {
        return pIndoorKilling;
    }

    void NodeVectorEventContextHost::ReleaseMosquitoes(
        NonNegativeFloat cost,
        const std::string& species,
        const VectorMatingStructure& genetics,
        uint32_t number
    )
    {
        IncrementCampaignCost( cost );
        INodeVector * pNV = nullptr;
        if( node->QueryInterface( GET_IID( INodeVector ), (void**)&pNV ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "Node", "INodeVector" );
        }
        pNV->AddVectors( species, genetics, number );
        return;
    }
}

#if 0
namespace Kernel
{
    template<class Archive>
    void serialize(Archive &ar, NodeVectorEventContextHost &context, const unsigned int v)
    {
        // Serialize base class
        ar & context.pLarvalKilling; // by habitat???
        ar & context.pLarvalHabitatReduction; // by habitat???
        ar & context.pVillageSpatialRepellent;
        ar & context.pADIVAttraction;
        ar & context.pADOVAttraction;
        ar & context.pPFVKill;
        ar & context.pOutdoorKilling;
        ar & context.pOutdoorKillingMale;
        ar & context.pSugarFeedKilling;
        ar & context.pOviTrapKilling; // by habitat???
        ar & context.pAnimalFeedKilling;
        ar & context.pOutdoorRestKilling;
        ar & context.larval_killing_target;
        ar & context.larval_reduction_target;
        ar & context.ovitrap_killing_target;
        //ar & context.larval_reduction;

        ar & context.pLarvalKilling; // by habitat???
        ar & context.pLarvalHabitatReduction; // by habitat???
        ar & context.pVillageSpatialRepellent;
        ar & context.pADIVAttraction;
        ar & context.pADOVAttraction;
        ar & context.pPFVKill;
        ar & context.pOutdoorKilling;
        ar & context.pOutdoorKillingMale;
        ar & context.pSugarFeedKilling;
        ar & context.pOviTrapKilling; // by habitat???
        ar & context.pAnimalFeedKilling;
        ar & context.pOutdoorRestKilling;

        ar & boost::serialization::base_object<Kernel::NodeEventContextHost>(context);
    }
}
#endif
