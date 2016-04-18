/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Debug.h"
#include "Exceptions.h"
#include "Sugar.h"
#include "Environment.h"
#include "InterventionsContainer.h"
#include "IIndividualHuman.h"                // for IIndividualHumanContext functions e.g. GetEventContext()
#include <typeinfo>
#ifndef WIN32
#include <cxxabi.h>
#endif
#include "NodeEventContext.h"
#include "INodeContext.h"

static const char* _module = "InterventionsContainer";

namespace Kernel
{
    QueryResult
    InterventionsContainer::QueryInterface( iid_t iid, void** ppinstance )
    {
        assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        GET_IID(IIndividualHumanInterventionsContext);
        iid_t retVal = GET_IID(IIndividualHumanInterventionsContext);
        if (iid == retVal)
            foundInterface = static_cast<IIndividualHumanInterventionsContext*>(this);
        else if (iid == GET_IID(IInterventionConsumer))
            foundInterface = static_cast<IInterventionConsumer*>(this);
        else if (iid == GET_IID(IVaccineConsumer))
            foundInterface = static_cast<IVaccineConsumer*>(this);
        else if (iid == GET_IID(IDrugVaccineInterventionEffects))
            foundInterface = static_cast<IDrugVaccineInterventionEffects*>(this);
        else if (iid == GET_IID(IPropertyValueChangerEffects))
            foundInterface = static_cast<IPropertyValueChangerEffects*>(this);
        else
            foundInterface = nullptr;

        QueryResult status;
        if ( !foundInterface )
        {
            status = e_NOINTERFACE;
            // if we had a base class we would QI down into it, but we don't.
        }
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    InterventionsContainer::~InterventionsContainer()
    {
        for (auto intervention : interventions)
        {
            delete intervention;
        }
    }

    void InterventionsContainer::PropagateContextToDependents()
    {
        IIndividualHumanContext *context = GetParent();
        for (auto intervention : interventions)
        {
            intervention->SetContextTo(context);
        }
    }

    std::list<IDistributableIntervention*> InterventionsContainer::GetInterventionsByType(const std::string &type_name)
    {
        std::list<IDistributableIntervention*> interventions_of_type;
        LOG_DEBUG_F( "Looking for intervention of type %s\n", type_name.c_str() );
        for (auto intervention : interventions)
        {
            std::string cur_iv_type_name = typeid( *intervention ).name();
#ifndef WIN32
            cur_iv_type_name = abi::__cxa_demangle(cur_iv_type_name.c_str(), 0, 0, nullptr );
#endif
            LOG_DEBUG_F("intervention name = %s\n", cur_iv_type_name.c_str());
            if( cur_iv_type_name == type_name )
            {
                LOG_DEBUG("Found one...\n");
                interventions_of_type.push_back( intervention );
            }
            /*else
            {
                LOG_INFO_F("No match: you asked about %s but I have %s\n", type_name, cur_iv_type_name);
            }*/
        }

        return interventions_of_type;
    }

    IDistributableIntervention* InterventionsContainer::GetIntervention( const std::string& iv_name )
    {
        for( auto p_intervention : interventions )
        {
            std::string cur_iv_type_name = typeid( *p_intervention ).name();
            if( cur_iv_type_name == iv_name )
            {
                return p_intervention ;
            }
        }
        return nullptr ;
    }

    void InterventionsContainer::PurgeExisting(
        const std::string &iv_name
    )
    {
        IDistributableIntervention* p_intervention = GetIntervention( iv_name );
        if( p_intervention != nullptr )
        {
            LOG_DEBUG_F("Found an existing intervention by that name (%s) which we are purging\n", iv_name.c_str());
            interventions.remove( p_intervention );
            delete p_intervention;
        }
    }

    bool InterventionsContainer::ContainsExisting( const std::string &iv_name )
    {
        IDistributableIntervention* p_intervention = GetIntervention( iv_name );
        return (p_intervention != nullptr);
    }

    void InterventionsContainer::Update(float dt)
    {
        drugVaccineReducedAcquire   = 1.0;
        drugVaccineReducedTransmit  = 1.0;
        drugVaccineReducedMortality = 1.0;

        if ( !interventions.empty() )
        {
            for (auto intervention : interventions)
            {
                intervention->Update(dt);
            }

            // TODO: appears that it might be more efficient to remove on the fly
            std::list<IDistributableIntervention*> dead_ivs;
            for (auto intervention : interventions)
            {
                if( intervention->Expired() )
                {
                    LOG_DEBUG("Found an expired intervention\n");
                    dead_ivs.push_back( intervention );
                }
            }

            // Remove any expired interventions (e.g., calendars). Don't want these things accumulating forever.
            for (auto intervention : dead_ivs)
            {
                LOG_DEBUG("Destroying an expired intervention.\n");
                interventions.remove( intervention );
                //pIV->Release(); // is refcounting implemented on interventions?
                delete intervention;
            }
        }
    }

    InterventionsContainer::InterventionsContainer()
        : drugVaccineReducedAcquire(1.0f)
        , drugVaccineReducedTransmit(1.0f)
        , drugVaccineReducedMortality(1.0f)
        , parent(nullptr)
    {
    }

    bool InterventionsContainer::GiveIntervention(
        IDistributableIntervention* iv
    )
    {
        interventions.push_back( iv );
        // We need to increase the reference counter here to represent fact that interventions container
        // is keeping a pointer to the intervention. (Otherwise when event coordinator calls Release,
        // and ref counter is decremented, the intervention object will delete itself.)
        iv->AddRef();
        iv->SetContextTo( parent );
        // TODO: For vaccine, now vaccine intervention needs to call ApplyVaccineTake on itself (???)
        LOG_DEBUG_F("InterventionsContainer has %d interventions now\n", interventions.size());
        return true;
    }

    void InterventionsContainer::UpdateVaccineAcquireRate(
        float acquire
    )
    {
        drugVaccineReducedAcquire *= (1.0f-acquire);
    }

    void InterventionsContainer::UpdateVaccineTransmitRate(
        float xmit
    )
    {
        drugVaccineReducedTransmit *= (1.0f-xmit);
    }

    void InterventionsContainer::UpdateVaccineMortalityRate(
        float mort
    )
    {
        drugVaccineReducedMortality *= (1.0f-mort);
    }

    void InterventionsContainer::ChangeProperty(
        const char * property,
        const char * new_value
    )
    {
/*
        LOG_DEBUG_F( "ChangeProperty: property = %s, new_value = %s\n", property, new_value );
        if( strlen( property ) == 0 )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ChangeProperty called with empty property string." );
        }
        if( strlen( new_value ) == 0 )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "ChangeProperty called with empty value string." );
        }
*/
        // Get parent property (remove need for casts)
        tProperties* pProps = parent->GetEventContext()->GetProperties();
        release_assert( pProps );
        // Check that property exists, except Age_Bins which are special case. We bootstrap individuals into age_bins at t=1,
        // with no prior existing age bin property.
        if( ( std::string( property ) != "Age_Bin" ) && ( pProps->find( property ) == pProps->end() ) )
        {
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", property );
        }

        INodeContext* pNode = nullptr;
        if ( s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeContext), (void**)&pNode) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeContext", "INodeEventContext" );
        }
        pNode->checkValidIPValue( property, new_value );
        //dynamic_cast<INodeContext*>(parent->GetEventContext()->GetNodeEventContext())->checkValidIPValue( property, new_value );

        if( (*pProps)[ property ] != new_value )
        {
            LOG_DEBUG_F( "Moving individual (%lu) property %s from %s to %s\n", parent->GetSuid().data, property, (*pProps)[ property ].c_str(), new_value );
            parent->UpdateGroupPopulation(-1.0f);
            (*pProps)[ property ] = new_value;
            parent->UpdateGroupMembership();
            parent->UpdateGroupPopulation(1.0f);
            parent->SetPropertyReportString("");
        }
        else
        {
            LOG_WARN_F( "ChangeProperty found that individual %lu already has property value %s.\n", parent->GetSuid().data, new_value );
        }
    }

    void
    InterventionsContainer::SetContextTo(
        IIndividualHumanContext* context
    )
    {
        parent = context;
        if (parent)
        {
            PropagateContextToDependents();
        }
    }

    IIndividualHumanContext*
    InterventionsContainer::GetParent()
    {
        return parent;
    }

    float InterventionsContainer::GetInterventionReducedAcquire()   const { return drugVaccineReducedAcquire; }
    float InterventionsContainer::GetInterventionReducedTransmit()  const {
        return drugVaccineReducedTransmit;
    }
    float InterventionsContainer::GetInterventionReducedMortality() const { return drugVaccineReducedMortality; }

    REGISTER_SERIALIZABLE(InterventionsContainer);

    void InterventionsContainer::serialize(IArchive& ar, InterventionsContainer* obj)
    {
        InterventionsContainer& container = *obj;
        ar.labelElement("drugVaccineReducedAcquire") & container.drugVaccineReducedAcquire;
        ar.labelElement("drugVaccineReducedTransmit") & container.drugVaccineReducedTransmit;
        ar.labelElement("drugVaccineReducedMortality") & container.drugVaccineReducedMortality;
        ar.labelElement("interventions") & container.interventions;
    }
}
