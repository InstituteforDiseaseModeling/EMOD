/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVSimpleDiagnostic.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function and access into IHIVCascadeOfCare
#include "IIndividualHumanHIV.h"  // for IndividualHIV access

static const char * _module = "HIVSimpleDiagnostic";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVSimpleDiagnostic, SimpleDiagnostic)
        HANDLE_INTERFACE(IHIVCascadeStateIntervention)
    END_QUERY_INTERFACE_DERIVED(HIVSimpleDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVSimpleDiagnostic)

    HIVSimpleDiagnostic::HIVSimpleDiagnostic()
    : SimpleDiagnostic() // true implies only support events in SimpleDiagnostic
    , abortStates()
    , cascadeState("")
    , firstUpdate(true)
    , result_of_positive_test(false)
    , original_days_to_diagnosis(0.0)
    , negative_diagnosis_event()
    {
        initSimTypes(1, "HIV_SIM");

        // in a refactor, these might be lifted to a common HIVIntervention class
        abortStates.value_source = "Valid_Cascade_States.*";

        initConfigTypeMap("Abort_States", &abortStates, HIV_Abort_States_DESC_TEXT);
        initConfigTypeMap("Cascade_State", &cascadeState, HIV_Cascade_State_DESC_TEXT);
        initConfigTypeMap("Days_To_Diagnosis", &days_to_diagnosis, SD_Days_To_Diagnosis_DESC_TEXT, 0, FLT_MAX, 0);
    }

    HIVSimpleDiagnostic::HIVSimpleDiagnostic( const HIVSimpleDiagnostic& master )
    : SimpleDiagnostic( master )
    {
        abortStates = master.abortStates;
        cascadeState = master.cascadeState;
        firstUpdate = master.firstUpdate;
        result_of_positive_test = master.result_of_positive_test;
        original_days_to_diagnosis = master.original_days_to_diagnosis;
        negative_diagnosis_event = master.negative_diagnosis_event;
    }

    bool HIVSimpleDiagnostic::Configure(const Configuration * inputJson)
    {
        if( getEventOrConfig( inputJson ) == EventOrConfig::Event || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Negative_Diagnosis_Event", &negative_diagnosis_event, HIV_SD_Negative_Diagnosis_Event_DESC_TEXT );
        }

        ConfigurePositiveEventOrConfig( inputJson );
        bool ret = JsonConfigurable::Configure(inputJson);
        if( ret )
        {
            // error if the cascadeState is an abortState
            if (abortStates.find(cascadeState) != abortStates.end())
            {
                std::string abort_state_list ;
                for( auto state : abortStates )
                {
                    abort_state_list += state + ", " ;
                }
                if( abortStates.size() > 0 )
                {
                    abort_state_list = abort_state_list.substr( 0, abort_state_list.length() - 2 );
                }
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "Cascade_State", cascadeState.c_str(),
                                                        "Abort_States", abort_state_list.c_str(),
                                                        "The Cascade_State cannot be one of the Abort_States." );
            }

            //CheckPostiveEventConfig();
        }
        return ret ;
    }

    EventOrConfig::Enum
    HIVSimpleDiagnostic::getEventOrConfig(
        const Configuration * inputJson
    )
    {
        // For those premature optimizers out there, this function is expected to get more interesting in the future.
        return EventOrConfig::Event;
    }

    bool HIVSimpleDiagnostic::qualifiesToGetIntervention( IIndividualHumanContext* pIndivid )
    {
        return !AbortDueToCurrentCascadeState();
    }

    bool HIVSimpleDiagnostic::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * const pICCO
    )
    {
        parent = context->GetParent();
        LOG_DEBUG_F( "Individual %d is getting tested.\n", parent->GetSuid().data );

        if( qualifiesToGetIntervention( parent ) )
        {
            return BaseIntervention::Distribute( context, pICCO );
        }
        else
        {
            expired = true ;
            return false ;
        }
    }

    void HIVSimpleDiagnostic::ActOnResultsIfTime()
    {
        // This can happen immediately if days_to_diagnosis is initialized to zero.
        if ( days_to_diagnosis <= 0 )
        {
            if( result_of_positive_test )
            {
                LOG_DEBUG_F( "Individual %d tested positive.\n", parent->GetSuid().data );
                if( SMART_DRAW( treatment_fraction ) )
                {
                    positiveTestDistribute();
                }
                else
                {
                    // this person doesn't get the positive test result
                    // because they defaulted / don't want treatment
                    onPatientDefault();
                    expired = true;
                }
            }
            else
            {
                LOG_DEBUG_F( "Individual %d tested negative.\n", parent->GetSuid().data );
                onNegativeTestResult();
            }
        }
    }

    void
    HIVSimpleDiagnostic::onNegativeTestResult()
    {
        auto iid = parent->GetSuid().data;
        LOG_DEBUG_F( "Individual %d tested 'negative' in HIVSimpleDiagnostic, receiving actual intervention.\n", iid );

        if( (negative_diagnosis_event != NO_TRIGGER_STR) && !negative_diagnosis_event.IsUninitialized() )
        {
            LOG_DEBUG_F( "Brodcasting event %s as negative diagnosis event for individual %d.", negative_diagnosis_event.c_str(), iid );
            broadcastEvent( negative_diagnosis_event );
        }
        else
        {
            LOG_DEBUG_F( "Negative diagnosis event is NoTrigger for individual %d.\n", iid );
        }
        expired = true;
    }


    bool HIVSimpleDiagnostic::AbortDueToCurrentCascadeState()
    {
        IHIVCascadeOfCare *ihcc = nullptr;
        if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVCascadeOfCare), (void **)&ihcc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent->GetInterventionsContext()",
                                           "IHIVCascadeOfCare",
                                           "IIndividualHumanInterventionsContext" );
        }

        std::string currentState = ihcc->getCascadeState();

        if ( abortStates.find(currentState) != abortStates.end() )
        {

            // Duplicated from SimpleDiagnostic::positiveTestDistribute
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (s_OK != parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), (void**)&broadcaster))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }

            broadcaster->TriggerNodeEventObservers( parent->GetEventContext(), IndividualEventTriggerType::CascadeStateAborted );
            LOG_DEBUG_F("The current cascade state \"%s\" is one of the Abort_States.  Expiring the diagnostic for individual %d.\n", currentState.c_str(), parent->GetSuid().data );
            expired = true;

            return true;
        }
        return false;
    }

    // todo: lift to HIVIntervention or helper function (repeated in HIVDelayedIntervention)
    bool HIVSimpleDiagnostic::UpdateCascade()
    {
        if( AbortDueToCurrentCascadeState() )
        {
            return false ;
        }

        // is this the first time through?  if so, update the cascade state.
        if (firstUpdate)
        {
            IHIVCascadeOfCare *ihcc = nullptr;
            if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVCascadeOfCare), (void **)&ihcc) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "parent->GetInterventionsContext()",
                                               "IHIVCascadeOfCare",
                                               "IIndividualHumanInterventionsContext" );
            }
            LOG_DEBUG_F( "Setting Cascade State to %s for individual %d.\n", cascadeState.c_str(), parent->GetSuid().data );
            ihcc->setCascadeState(cascadeState);

            original_days_to_diagnosis = days_to_diagnosis;
        }
        return true ;
    }


    void HIVSimpleDiagnostic::Update( float dt )
    {
        bool cascade_state_ok = UpdateCascade();
        if( !cascade_state_ok )
        {
            // the cascadeState must be an abort state
            return ;
        }

        if( firstUpdate )
        {
            result_of_positive_test = positiveTestResult() ;
        }
        else
        {
            // ------------------------------------------------------------------------------
            // --- Count down the time until a positive test result comes back
            // ---    Update() is called the same day as Distribute() so we don't want
            // ---    to decrement the counter until the next day.
            // ------------------------------------------------------------------------------
            days_to_diagnosis -= dt;
        }

        ActOnResultsIfTime();

        firstUpdate = false;
    }

    const std::string& HIVSimpleDiagnostic::GetCascadeState()
    {
        return cascadeState;
    }

    const jsonConfigurable::tDynamicStringSet& HIVSimpleDiagnostic::GetAbortStates()
    {
        return abortStates;
    }

    REGISTER_SERIALIZABLE(HIVSimpleDiagnostic);

    void HIVSimpleDiagnostic::serialize(IArchive& ar, HIVSimpleDiagnostic* obj)
    {
        SimpleDiagnostic::serialize( ar, obj );
        HIVSimpleDiagnostic& hsd = *obj;
        ar.labelElement("abortStates"               ) & hsd.abortStates;
        ar.labelElement("cascadeState"              ) & hsd.cascadeState;
        ar.labelElement("firstUpdate"               ) & hsd.firstUpdate;
        ar.labelElement("result_of_positive_test"   ) & hsd.result_of_positive_test;
        ar.labelElement("original_days_to_diagnosis") & hsd.original_days_to_diagnosis;
        ar.labelElement("absoluteDuration"          ) & hsd.absoluteDuration;
        ar.labelElement("negative_diagnosis_event"  ) & hsd.negative_diagnosis_event;
    }
}
