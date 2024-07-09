
#include "stdafx.h"
#include "ControlledVaccine.h"
#include "Interventions.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "ControlledVaccine" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(ControlledVaccine,SimpleVaccine)
        HANDLE_INTERFACE(IControlledVaccine)
    END_QUERY_INTERFACE_DERIVED(ControlledVaccine,SimpleVaccine)

    IMPLEMENT_FACTORY_REGISTERED(ControlledVaccine)

    bool
    ControlledVaccine::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Distributed_Event_Trigger",             &m_DistributedEventTrigger,           RV_Distributed_Event_Trigger_DESC_TEXT );
        initConfigTypeMap( "Expired_Event_Trigger",                 &m_ExpiredEventTrigger,               RV_Expired_Event_Trigger_DESC_TEXT     );
        initConfigTypeMap( "Duration_To_Wait_Before_Revaccination", &m_DurationToWaitBeforeRevaccination, RV_Duration_To_Wait_Before_Revaccination_DESC_TEXT, 0, FLT_MAX, FLT_MAX);

        bool configured = SimpleVaccine::Configure( inputJson );
        return configured;
    }

    ControlledVaccine::ControlledVaccine() 
    : SimpleVaccine()
    , m_DurationToWaitBeforeRevaccination(FLT_MAX)
    , m_TimeSinceVaccination(0.0)
    , m_DistributedEventTrigger()
    , m_ExpiredEventTrigger()
    {
    }

    ControlledVaccine::ControlledVaccine( const ControlledVaccine& master )
    : SimpleVaccine( master )
    , m_DurationToWaitBeforeRevaccination( master.m_DurationToWaitBeforeRevaccination )
    , m_TimeSinceVaccination(              master.m_TimeSinceVaccination              )
    , m_DistributedEventTrigger(           master.m_DistributedEventTrigger           )
    , m_ExpiredEventTrigger(               master.m_ExpiredEventTrigger               )
    {
    }

    ControlledVaccine::~ControlledVaccine()
    {
    }

    bool ControlledVaccine::Distribute( IIndividualHumanInterventionsContext *context, ICampaignCostObserver * pCCO )
    {
        // ------------------------------------------------------------------------------------------------
        // --- Check if the person is already vaccinated.  If they are see if we should revaccinate them.
        // --- For example, if the vaccine becomes ineffective after two years, you might want to get
        // --- re-vaccinated at 18-months to ensure you have good coverage.
        // ------------------------------------------------------------------------------------------------
        bool distribute = true;
        std::list<void*> allowable_list = context->GetInterventionsByInterface( GET_IID(IControlledVaccine) );
        for( auto p_allow : allowable_list )
        {
            IControlledVaccine* p_existing_vaccine = static_cast<IControlledVaccine*>(p_allow);
            if( !p_existing_vaccine->AllowRevaccination( *this ) )
            {
                distribute = false;
                break;
            }
        }

        if( distribute )
        {
            distribute = SimpleVaccine::Distribute( context, pCCO );
        }

        if( distribute && !m_DistributedEventTrigger.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = context->GetParent()->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( context->GetParent()->GetEventContext(), m_DistributedEventTrigger );
        }

        return distribute;
    }

    void ControlledVaccine::Update( float dt )
    {
        SimpleVaccine::Update( dt );
        m_TimeSinceVaccination += dt;

        if( expired && !m_ExpiredEventTrigger.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( parent->GetEventContext(), m_ExpiredEventTrigger );
        }
    }

    const InterventionName& ControlledVaccine::GetInterventionName() const
    {
        return GetName();
    }

    bool ControlledVaccine::AllowRevaccination( const IControlledVaccine& rNewVaccine ) const
    {
        bool allow = false;
        if( expired )
        {
            // This is to cover the situation where there is a listener to the expired event
            // and that listener tries to add a new vaccine.  The vacine will have expired
            // but it will not have been removed.
            allow = true;
        }
        else if( (m_TimeSinceVaccination >= m_DurationToWaitBeforeRevaccination) ||
                 (this->GetInterventionName() != rNewVaccine.GetInterventionName()) )
        {
            allow = true;
        }
        return allow;
    }

    REGISTER_SERIALIZABLE(ControlledVaccine);

    void ControlledVaccine::serialize(IArchive& ar, ControlledVaccine* obj)
    {
        SimpleVaccine::serialize( ar, obj );
        ControlledVaccine& vaccine = *obj;
        ar.labelElement( "m_TimeSinceVaccination"              ) & vaccine.m_TimeSinceVaccination;
        ar.labelElement( "m_DurationToWaitBeforeRevaccination" ) & vaccine.m_DurationToWaitBeforeRevaccination;
        ar.labelElement( "m_DistributedEventTrigger"           ) & vaccine.m_DistributedEventTrigger;
        ar.labelElement( "m_ExpiredEventTrigger"               ) & vaccine.m_ExpiredEventTrigger;
    }
}
