
#include <stdafx.h>
#ifndef WIN32
#include <cxxabi.h>
#endif

#include "Interventions.h"
#include "InterventionFactory.h"
#include "INodeContext.h"
#include "NodeEventContext.h"
#include "IIndividualHumanContext.h"
#include "Exceptions.h"
#include "EventTrigger.h"

#include "RapidJsonImpl.h"

SETUP_LOGGING( "Interventions" )

//unsigned int total_intervention_counter = 0;

namespace Kernel
{
    BaseIntervention::BaseIntervention()
        : parent( nullptr )
        , name()
        , cost_per_unit(0.0f)
        , expired(false)
        , dont_allow_duplicates(false)
        , first_time(true)
        , disqualifying_properties()
        , status_property()
    {
        initSimTypes( 1, "*" );

        //total_intervention_counter++;
        //LOG_DEBUG_F("New intervention, total_intervention_counter = %d\n", total_intervention_counter);
    }

    BaseIntervention::BaseIntervention( const BaseIntervention& master )
        : JsonConfigurable()
        , parent(nullptr)
        , name( master.name )
        , cost_per_unit( master.cost_per_unit )
        , expired( master.expired )
        , dont_allow_duplicates( master.dont_allow_duplicates )
        , first_time( master.first_time )
        , disqualifying_properties( master.disqualifying_properties )
        , status_property( master.status_property )
    {
    }

    BaseIntervention::~BaseIntervention()
    {
        //total_intervention_counter--;
        //LOG_DEBUG_F("Deleted intervention, total_intervention_counter = %d\n", total_intervention_counter);
    }

    bool BaseIntervention::Configure(const Configuration * inputJson)
    {
        // ------------------------------------------------------------------
        // --- Must calculate default name in Configure(). You can't do it
        // --- in the constructor because the pointer doesn't know what object
        // --- it is yet.
        // ------------------------------------------------------------------
        std::string default_name = typeid(*this).name();
#ifdef WIN32
        default_name = default_name.substr( 14 ); // remove "class Kernel::"
#else
        default_name = abi::__cxa_demangle( default_name.c_str(), 0, 0, nullptr );
        default_name = default_name.substr( 8 ); // remove "Kernel::"
#endif


        std::string tmp_name = default_name;
        initConfigTypeMap( "Intervention_Name", &tmp_name, Intervention_Name_DESC_TEXT, default_name );

        initConfigTypeMap( "Dont_Allow_Duplicates", &dont_allow_duplicates, IND_Dont_Allow_Duplicates_DESC_TEXT, false );

        IPKeyValueParameter ip_key_value;
        jsonConfigurable::tDynamicStringSet tmp_disqualifying_properties;
        initConfigTypeMap("Disqualifying_Properties", &tmp_disqualifying_properties, IP_Disqualifying_Properties_DESC_TEXT );
        initConfigTypeMap("New_Property_Value", &ip_key_value, IP_New_Property_Value_DESC_TEXT );

        bool ret = JsonConfigurable::Configure(inputJson);
        if( ret && !JsonConfigurable::_dryrun )
        {
            name = tmp_name;

            // transfer the strings into the IPKeyValueContainer
            for( auto state : tmp_disqualifying_properties )
            {
                IPKeyValue kv( state );
                disqualifying_properties.Add( kv );
            }

            // error if the intervention_state is an invalid_state
            status_property = ip_key_value;
            if( status_property.IsValid() && disqualifying_properties.Contains( status_property ) )
            {
                std::string abort_state_list ;
                for( auto state : tmp_disqualifying_properties )
                {
                    abort_state_list += "'" + state + "', " ;
                }
                if( tmp_disqualifying_properties.size() > 0 )
                {
                    abort_state_list = abort_state_list.substr( 0, abort_state_list.length() - 2 );
                }
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "New_Property_Value", status_property.ToString().c_str(),
                                                        "Disqualifying_Properties", abort_state_list.c_str(),
                                                        "The New_Property_Value cannot be one of the Disqualifying_Properties." );
            }

            if( name.empty() )
            {
                std::stringstream ss;
                ss << "Invalid Parameter Value\n";
                ss << "In '" << GetTypeName() << "', you cannot set the parameter 'Intervention_Name' to empty string.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return ret ;
    }

    bool
    BaseIntervention::Expired()
    {
        return expired;
    }

    void BaseIntervention::SetExpired( bool isExpired )
    {
        expired = isExpired;
    }

    bool
    BaseIntervention::Distribute(
        IIndividualHumanInterventionsContext *context, // interventions container usually
        ICampaignCostObserver * const pICCO
    )
    {
        if( dont_allow_duplicates && context->ContainsExistingByName( name ) )
        {
            return false;
        }

        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        bool wasDistributed=false;
        IInterventionConsumer * ic;
        if (s_OK == (context->QueryInterface(GET_IID(IInterventionConsumer), (void**)&ic) ) )
        {
            assert(ic);
            if( ic->GiveIntervention(this) )
            {
                // Need to get Individual pointer from interventions container pointer. Try parent.
                IIndividualHumanEventContext * pIndiv = (context->GetParent())->GetEventContext();
                if( pICCO )
                {
                    pICCO->notifyCampaignExpenseIncurred( cost_per_unit, pIndiv );
                }
                wasDistributed = true;
            }
        } 
        else
        {
            std::ostringstream oss;
            oss << "Unable to distribute intervention because IIndividualHumanInterventionsContext doesn't support IInterventionConsumer." << std::endl;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
        }
        return wasDistributed;
    }

    bool BaseIntervention::AbortDueToDisqualifyingInterventionStatus( IIndividualHumanContext* pHuman )
    {
        const IPKeyValueContainer* props = pHuman->GetEventContext()->GetProperties();

        IPKeyValue found = props->FindFirst( disqualifying_properties );
        if( found.IsValid() )
        {
            LOG_DEBUG_F( "The property \"%s\" for intervention is one of the Disqualifying_Properties.  Expiring '%s' for individual %d.\n",
                            found.ToString().c_str(), name.c_str(), pHuman->GetSuid().data );

            IIndividualEventBroadcaster* broadcaster = pHuman->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();

            broadcaster->TriggerObservers( pHuman->GetEventContext(), EventTrigger::InterventionDisqualified );
            expired = true;

            return true;
        }
        return false;
    }

    bool BaseIntervention::UpdateIndividualsInterventionStatus( bool checkDisqualifyingProperties )
    {
        if( checkDisqualifyingProperties && AbortDueToDisqualifyingInterventionStatus( parent ) )
        {
            return false;
        }
        // is this the first time through?  if so, update the intervention state.
        if( first_time && status_property.IsValid() )
        {
            LOG_DEBUG_F( "Setting Intervention Status to %s for individual %d.\n", status_property.ToString().c_str(), parent->GetSuid().data );
            parent->GetInterventionsContext()->ChangeProperty( status_property.GetKeyAsString().c_str(), status_property.GetValueAsString().c_str() );
            first_time = false;
        }
        return true;
    }

    void BaseIntervention::SetContextTo( IIndividualHumanContext *context )
    {
        parent = context;
    }

    ReportInterventionData BaseIntervention::GetReportInterventionData() const
    {
        ReportInterventionData data( true, GetName().ToString() );
        return data;
    }

    void BaseIntervention::serialize( IArchive& ar, BaseIntervention* obj )
    {
        BaseIntervention& intervention = *obj;
        ar.labelElement( "name" ) & intervention.name;
        ar.labelElement( "cost_per_unit" ) & intervention.cost_per_unit;
        ar.labelElement( "expired" ) & intervention.expired;
        ar.labelElement( "dont_allow_duplicates" ) & intervention.dont_allow_duplicates;
        ar.labelElement( "first_time" ) & intervention.first_time;
        ar.labelElement( "disqualifying_properties" ) & intervention.disqualifying_properties;
        ar.labelElement( "status_property" ) & intervention.status_property;
    }

    // ------------------------------------------------------------------------
    // --- BaseNodeIntervention
    // ------------------------------------------------------------------------

    BaseNodeIntervention::BaseNodeIntervention()
        : parent(nullptr)
        , name()
        , cost_per_unit(0.0f)
        , expired(false)
        , dont_allow_duplicates(false)
        , first_time(true)
        , disqualifying_properties()
        , status_property()
    {
        initSimTypes( 1, "*" );
    }

    bool BaseNodeIntervention::Configure( const Configuration * inputJson )
    {
        // ------------------------------------------------------------------
        // --- Must calculate default name in Configure(). You can't do it
        // --- in the constructor because the pointer doesn't know what object
        // --- it is yet.
        // ------------------------------------------------------------------
        std::string default_name = typeid(*this).name();
#ifdef WIN32
        default_name = default_name.substr( 14 ); // remove "class Kernel::"
#else
        default_name = abi::__cxa_demangle( default_name.c_str(), 0, 0, nullptr );
        default_name = default_name.substr( 8 ); // remove "Kernel::"
#endif
        std::string tmp_name = default_name;

        initConfigTypeMap( "Intervention_Name", &tmp_name, Intervention_Name_DESC_TEXT, default_name );
        initConfigTypeMap( "Dont_Allow_Duplicates", &dont_allow_duplicates, NODE_Dont_Allow_Duplicates_DESC_TEXT, false );

        NPKeyValueParameter np_key_value;
        jsonConfigurable::tDynamicStringSet tmp_disqualifying_properties;
        initConfigTypeMap( "Disqualifying_Properties", &tmp_disqualifying_properties, NP_Disqualifying_Properties_DESC_TEXT );
        initConfigTypeMap( "New_Property_Value", &np_key_value, NP_New_Property_Value_DESC_TEXT );

        bool ret = JsonConfigurable::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            name = tmp_name;

            // transfer the strings into the NPKeyValueContainer
            for( auto state : tmp_disqualifying_properties )
            {
                NPKeyValue kv( state );
                disqualifying_properties.Add( kv );
            }

            // error if the intervention_state is an invalid_state
            status_property = np_key_value;
            if( status_property.IsValid() && disqualifying_properties.Contains( status_property ) )
            {
                std::string abort_state_list ;
                for( auto state : tmp_disqualifying_properties )
                {
                    abort_state_list += "'" + state + "', " ;
                }
                if( tmp_disqualifying_properties.size() > 0 )
                {
                    abort_state_list = abort_state_list.substr( 0, abort_state_list.length() - 2 );
                }
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "New_Property_Value", status_property.ToString().c_str(),
                                                        "Disqualifying_Properties", abort_state_list.c_str(),
                                                        "The New_Property_Value cannot be one of the Disqualifying_Properties." );
            }
        }
        return ret ;
    }

    bool
    BaseNodeIntervention::Expired()
    {
        return expired;
    }

    void BaseNodeIntervention::SetExpired( bool isExpired )
    {
        expired = isExpired;
    }

    bool
    BaseNodeIntervention::Distribute(
        INodeEventContext *context, // interventions container usually
        IEventCoordinator2 * ec
    )
    {
        parent = context;

        if( dont_allow_duplicates && context->ContainsExistingByName(name) )
        {
            return false;
        }

        if( AbortDueToDisqualifyingInterventionStatus( context ) )
        {
            return false;
        }

        bool wasDistributed=false;
        INodeInterventionConsumer * ic;
        if (s_OK == (context->QueryInterface(GET_IID(INodeInterventionConsumer), (void**)&ic) ) )
        {
            assert(ic);
            if( ic->GiveIntervention(this) )
            {
                wasDistributed = true;
            }
        } 
        else
        {
            std::ostringstream oss;
            oss << "Unable to distribute intervention because INodeEventContext doesn't support INodeInterventionConsumer." << std::endl;
            throw IllegalOperationException(  __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
        }
        return wasDistributed;
    }

    void BaseNodeIntervention::SetContextTo( INodeEventContext *context )
    {
        parent = context;
    }

    ReportInterventionData BaseNodeIntervention::GetReportInterventionData() const
    {
        ReportInterventionData data( false, GetName().ToString() );
        return data;
    }

    bool BaseNodeIntervention::AbortDueToDisqualifyingInterventionStatus( INodeEventContext* context )
    {
        NPKeyValueContainer& node_propeties = context->GetNodeContext()->GetNodeProperties();

        NPKeyValue found = node_propeties.FindFirst( disqualifying_properties );
        if( found.IsValid() )
        {
            LOG_DEBUG_F( "The property \"%s\" is one of the Disqualifying_Properties.  Expiring the '%s' for node %d.\n",
                found.ToString().c_str(), name.c_str(), context->GetExternalId() );

            IIndividualEventBroadcaster* broadcaster = context->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( nullptr, EventTrigger::InterventionDisqualified );
            expired = true;

            return true;
        }
        return false;
    }

    bool BaseNodeIntervention::UpdateNodesInterventionStatus()
    {
        if( AbortDueToDisqualifyingInterventionStatus( parent ) )
        {
            return false;
        }
        // is this the first time through?  if so, update the intervention state.
        if( first_time && status_property.IsValid() )
        {
            LOG_DEBUG_F( "Setting Intervention Status to %s for node %d.\n", status_property.ToString().c_str(), parent->GetExternalId() );
            parent->GetNodeContext()->GetNodeProperties().Set( status_property );
            first_time = false;
        }
        return true;
    }
}
