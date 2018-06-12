/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
#include "Properties.h"

SETUP_LOGGING( "InterventionsContainer" )

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

    std::list<IDistributableIntervention*> InterventionsContainer::GetInterventionsByName(const std::string &intervention_name)
    {
        std::list<IDistributableIntervention*> interventions_list;
        LOG_DEBUG_F( "Looking for interventions with name %s\n", intervention_name.c_str() );
        for (auto intervention : interventions)
        {
            if( intervention->GetName() == intervention_name )
            {
                interventions_list.push_back( intervention );
            }
        }

        return interventions_list;
    }

    std::list<void*> InterventionsContainer::GetInterventionsByInterface( iid_t iid )
    {
        std::list<void*> interface_list;
        for (auto intervention : interventions)
        {
            void* p_interface = nullptr;
            if ( s_OK == intervention->QueryInterface( iid, (void**)&p_interface) )
            {
                interface_list.push_back( p_interface );
            }
        }

        return interface_list;
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

    bool InterventionsContainer::ContainsExistingByName( const std::string& name )
    {
        for( auto intervention : interventions )
        {
            if( intervention->GetName() == name )
            {
                return true;
            }
        }
        return false;
    }

    void InterventionsContainer::InfectiousLoopUpdate( float dt )
    {
        // --- ------------------------------------------------------------------------------------
        // --- The purpose of this method is to update the interventions that need to be updated
        // --- in the infectious update loop.  These interventions can be updated multiple times
        // --- per time step and affect the person's infections.  Drugs are typically interventions
        // --- that need to be in this loop.
        // --- ------------------------------------------------------------------------------------
        drugVaccineReducedAcquire   = 1.0;
        drugVaccineReducedTransmit  = 1.0;
        drugVaccineReducedMortality = 1.0;

        for( auto intervention : interventions )
        {
            if( intervention->NeedsInfectiousLoopUpdate() )
            {
                intervention->Update( dt );
            }
        }
    }

    void InterventionsContainer::Update( float dt )
    {
        if( !interventions.empty() )
        {
            // -----------------------------------------------------------------------
            // --- The "orig_num" check below is done below so that we update
            // --- interventions that were distributed by the existing interventions.
            // --- We need to update both types of new interventions.
            // -----------------------------------------------------------------------
            int orig_num = interventions.size();
            int i = 0;
            for( auto intervention : interventions )
            {
                if( !intervention->NeedsInfectiousLoopUpdate() || (i >= orig_num) )
                {
                    intervention->Update( dt );
                }
                ++i;
            }

            // TODO: appears that it might be more efficient to remove on the fly
            std::vector<IDistributableIntervention*> dead_ivs;
            for( auto intervention : interventions )
            {
                // ------------------------------------------------------------------------------
                // --- BEWARE: Some interventions distribute other interventions or fire events
                // --- (which can cause interventions to be added) during the call to Expired().
                // ------------------------------------------------------------------------------
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
        , interventions()
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
        LOG_DEBUG_F("InterventionsContainer (individual %d) has %d interventions now\n", GetParent()->GetSuid().data, interventions.size());
        return true;
    }

    void InterventionsContainer::UpdateVaccineAcquireRate(
        float acquire,
        bool isMultiplicative
    )
    {
        if( isMultiplicative )
        {
            drugVaccineReducedAcquire *= (1.0f - acquire);
        }
        else
        {
            drugVaccineReducedAcquire -= acquire;
        }

        if( drugVaccineReducedAcquire > 1.0 )
        {
            drugVaccineReducedAcquire = 1.0;
        }
        else if( drugVaccineReducedAcquire < 0.0 )
        {
            LOG_WARN_F("drugVaccineReducedAcquire(=%f) < 0, setting to zero\n",drugVaccineReducedAcquire);
            drugVaccineReducedAcquire = 0.0;
        }
    }

    void InterventionsContainer::UpdateVaccineTransmitRate(
        float xmit,
        bool isMultiplicative
    )
    {
        if( isMultiplicative )
        {
            drugVaccineReducedTransmit *= (1.0f - xmit);
        }
        else
        {
            drugVaccineReducedTransmit -= xmit;
        }

        if( drugVaccineReducedTransmit > 1.0 )
        {
            drugVaccineReducedTransmit = 1.0;
        }
        else if( drugVaccineReducedTransmit < 0.0 )
        {
            LOG_WARN_F("drugVaccineReducedTransmit(=%f) < 0, setting to zero\n",drugVaccineReducedTransmit);
            drugVaccineReducedTransmit = 0.0;
        }
    }

    void InterventionsContainer::UpdateVaccineMortalityRate(
        float mort,
        bool isMultiplicative
    )
    {
        if( isMultiplicative )
        {
            drugVaccineReducedMortality *= (1.0f - mort);
        }
        else
        {
            drugVaccineReducedMortality -= mort;
        }

        if( drugVaccineReducedMortality > 1.0 )
        {
            drugVaccineReducedMortality = 1.0;
        }
        else if( drugVaccineReducedMortality < 0.0 )
        {
            LOG_WARN_F("drugVaccineReducedMortality(=%f) < 0, setting to zero\n",drugVaccineReducedMortality);
            drugVaccineReducedMortality = 0.0;
        }
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
        IPKeyValueContainer* pProps = parent->GetEventContext()->GetProperties();

        // Check that property exists, except Age_Bins which are special case. We bootstrap individuals into age_bins at t=1,
        // with no prior existing age bin property.
        IPKey key( property );
        if( ( std::string( property ) != "Age_Bin" ) && !pProps->Contains( key ) )
        {
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "properties", property );
        }

        IPKeyValue new_kv( property, new_value );

        if( !pProps->Contains( new_kv ) )
        {
            IPKeyValue old_kv = pProps->Get( key );
            LOG_DEBUG_F( "Moving individual (%lu) property %s from %s to %s\n", parent->GetSuid().data, property, old_kv.GetValueAsString().c_str(), new_value );
            parent->UpdateGroupPopulation(-1.0f);
            pProps->Set( new_kv );
            parent->UpdateGroupMembership();
            parent->UpdateGroupPopulation(1.0f);
            parent->SetPropertyReportString("");

            //broadcast that the individual changed properties
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if( s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface( GET_IID( INodeTriggeredInterventionConsumer ), (void**)&broadcaster ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
            LOG_DEBUG_F( "Individual %d changed property, broadcasting PropertyChange \n", parent->GetSuid().data );
            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), EventTrigger::PropertyChange );
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

    float InterventionsContainer::GetInterventionReducedAcquire()   const { return drugVaccineReducedAcquire;   }
    float InterventionsContainer::GetInterventionReducedTransmit()  const { return drugVaccineReducedTransmit;  }
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
