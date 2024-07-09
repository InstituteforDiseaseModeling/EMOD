
#include "stdafx.h"

#include "AntiretroviralTherapyFull.h"

#include "IIndividualHumanContext.h"
#include "IndividualEventContext.h"
#include "IHIVInterventionsContainer.h"  // for IHIVDrugEffectsApply methods
#include "IInfectionHIV.h"
#include "ISusceptibilityHIV.h"
#include "IIndividualHumanHIV.h"
#include "SimulationEnums.h"
#include "RANDOM.h"
#include "Debug.h"
#include "DistributionFactory.h"
#include "InterpolatedValueMap.h"
#include "NodeEventContext.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "AntiretroviralTherapyFull" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED( AntiretroviralTherapyFull, AntiretroviralTherapy )
    END_QUERY_INTERFACE_DERIVED( AntiretroviralTherapyFull, AntiretroviralTherapy )

    IMPLEMENT_FACTORY_REGISTERED(AntiretroviralTherapyFull)

    AntiretroviralTherapyFull::AntiretroviralTherapyFull()
        : AntiretroviralTherapy()
        , m_pHumanHIV( nullptr )
        , m_pDrugEffects( nullptr )
        , m_pDistributionTimeOn( nullptr )
        , m_RemainingDays( 0 )
        , m_StopArtEvent()
    {
        initSimTypes( 1, "HIV_SIM" );
        m_RemainingDays.handle = std::bind( &AntiretroviralTherapyFull::Callback, this, std::placeholders::_1 );
    }

    AntiretroviralTherapyFull::AntiretroviralTherapyFull( const AntiretroviralTherapyFull& rMaster)
        : AntiretroviralTherapy( rMaster )
        , m_pHumanHIV( rMaster.m_pHumanHIV )
        , m_pDrugEffects( rMaster.m_pDrugEffects )
        , m_pDistributionTimeOn( nullptr )
        , m_RemainingDays( rMaster.m_RemainingDays )
        , m_StopArtEvent( rMaster.m_StopArtEvent )
    {
        if( rMaster.m_pDistributionTimeOn != nullptr )
        {
            m_pDistributionTimeOn = rMaster.m_pDistributionTimeOn->Clone();
        }

        m_RemainingDays.handle = std::bind( &AntiretroviralTherapyFull::Callback, this, std::placeholders::_1 );
    }

    AntiretroviralTherapyFull::~AntiretroviralTherapyFull()
    {
        delete m_pDistributionTimeOn;
        m_pDistributionTimeOn = nullptr;
    }

    bool
    AntiretroviralTherapyFull::Configure(
        const Configuration * inputJson
    )
    {
        float param1 = 0.0;
        float param2 = 0.0;
        InterpolatedValueMap ivm;

        DistributionFunction::Enum time_on_dist_type( DistributionFunction::CONSTANT_DISTRIBUTION );
        initConfig( "Time_On_ART_Distribution", time_on_dist_type, inputJson, MetadataDescriptor::Enum( "Time_On_ART_Distribution", ARTFull_Time_On_ART_Distribution_DESC_TEXT, MDD_ENUM_ARGS( DistributionFunction ) ) );
        m_pDistributionTimeOn = DistributionFactory::CreateDistribution( this, time_on_dist_type, "Time_On_ART", inputJson );

        initConfigTypeMap( "Stop_ART_Event", &m_StopArtEvent, ARTFull_Stop_ART_Event_DESC_TEXT );

        bool result = AntiretroviralTherapy::Configure(inputJson);

        if (result && !JsonConfigurable::_dryrun)
        {
        }

        return result;
    }

    void AntiretroviralTherapyFull::CalculateDelay()
    {
        if( m_pDistributionTimeOn->GetIPiecewiseDistribution() )
        {
            auto year = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
            m_pDistributionTimeOn->GetIPiecewiseDistribution()->SetX( year );
            m_RemainingDays = m_pDistributionTimeOn->Calculate( parent->GetRng() );
        }
        else
        {
            m_RemainingDays = m_pDistributionTimeOn->Calculate( parent->GetRng() );
        }
    }

    bool AntiretroviralTherapyFull::CanDistribute()
    {
        return m_pHumanHIV->HasHIV() && !m_pHumanHIV->GetHIVInterventionsContainer()->OnArtQuery();
    }

    void AntiretroviralTherapyFull::DeterminePointers( IIndividualHumanInterventionsContext *context )
    {
        if( s_OK != context->GetParent()->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&m_pHumanHIV ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context->GetParent()", "IIndividualHumanHIV", "IIndividualHumanContext" );
        }

        if( s_OK != context->QueryInterface( GET_IID( IHIVDrugEffectsApply ), (void**)&m_pDrugEffects ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "context", "IHIVDrugEffectsApply", "IIndividualHumanInterventionsContext" );
        }
    }

    bool AntiretroviralTherapyFull::Distribute( IIndividualHumanInterventionsContext *context,
                                                ICampaignCostObserver * const pICCO )
    {
        DeterminePointers( context );

        bool distributed = CanDistribute();
        if( distributed )
        {
            distributed = BaseIntervention::Distribute( context, pICCO );
            if( distributed )
            {
                CalculateDelay();
            }
        }
        return distributed;
    }

    void AntiretroviralTherapyFull::Update( float dt )
    {
        size_t s = sizeof( AntiretroviralTherapyFull );
        if( BaseIntervention::UpdateIndividualsInterventionStatus() )
        {
            // Not disqualified, so normal flow
            if( CanDistribute() )
            {
                float duration = 0.0;
                if( m_IsActiveViralSuppression )
                {
                    duration = ComputeDurationFromEnrollmentToArtAidsDeath( parent, m_pHumanHIV );
                }

                m_pDrugEffects->GoOnART( m_IsActiveViralSuppression, m_DaysToAchieveSuppression, duration, m_MultiplierOnTransmission );
            }

            m_RemainingDays.Decrement( dt );
        }
        else
        {
            // Probably got here due to a disqualifying property
            Callback( dt );
        }
    }

    void AntiretroviralTherapyFull::Callback( float dt )
    {
        // we should be getting here because we are trying to expire the intervention
        if( !expired && m_pHumanHIV->GetHIVInterventionsContainer()->OnArtQuery() )
        {
            m_pDrugEffects->GoOffART();

            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( parent->GetEventContext(), m_StopArtEvent );
        }
        expired = true;
    }

    REGISTER_SERIALIZABLE(AntiretroviralTherapyFull);

    void AntiretroviralTherapyFull::serialize(IArchive& ar, AntiretroviralTherapyFull* obj)
    {
        AntiretroviralTherapy::serialize( ar, obj );
        AntiretroviralTherapyFull& art = *obj;
        ar.labelElement( "m_pDistributionTimeOn" ) & art.m_pDistributionTimeOn;
        ar.labelElement( "m_RemainingDays"       ) & art.m_RemainingDays;
        ar.labelElement( "m_StopArtEvent"        ) & art.m_StopArtEvent;
    }
}
