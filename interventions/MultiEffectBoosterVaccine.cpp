
#include "stdafx.h"
#include "MultiEffectBoosterVaccine.h"
#include "InterventionsContainer.h"  // for IVaccineConsumer methods
#include "ISusceptibilityContext.h"
#include "IIndividualHumanContext.h"

SETUP_LOGGING( "MultiEffectBoosterVaccine" );

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(MultiEffectBoosterVaccine)

    bool
    MultiEffectBoosterVaccine::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Prime_Acquire",                  &prime_acquire,    MEBV_Prime_Acquire_DESC_TEXT,   0.0, 1.0, 0.0 ); 
        initConfigTypeMap("Prime_Transmit",                 &prime_transmit,   MEBV_Prime_Transmit_DESC_TEXT,  0.0, 1.0, 0.0 ); 
        initConfigTypeMap("Prime_Mortality",                &prime_mortality,  MEBV_Prime_Mortality_DESC_TEXT, 0.0, 1.0, 0.0 ); 
        initConfigTypeMap("Boost_Acquire",                  &boost_acquire,    MEBV_Boost_Acquire_DESC_TEXT,   0.0, 1.0, 0.0 ); 
        initConfigTypeMap("Boost_Transmit",                 &boost_transmit,   MEBV_Boost_Transmit_DESC_TEXT,  0.0, 1.0, 0.0 ); 
        initConfigTypeMap("Boost_Mortality",                &boost_mortality,  MEBV_Boost_Mortality_DESC_TEXT, 0.0, 1.0, 0.0 ); 
        initConfigTypeMap("Boost_Threshold_Acquire",        &boost_threshold_acquire,  MEBV_Boost_Threshold_Acquire_DESC_TEXT,  0.0, 1.0, 0.0 );
        initConfigTypeMap("Boost_Threshold_Transmit",       &boost_threshold_acquire,  MEBV_Boost_Threshold_Transmit_DESC_TEXT,  0.0, 1.0, 0.0 );
        initConfigTypeMap("Boost_Threshold_Mortality",      &boost_threshold_acquire,  MEBV_Boost_Threshold_Mortality_DESC_TEXT,  0.0, 1.0, 0.0 );

        bool configured = MultiEffectVaccine::Configure( inputJson );
        return configured;
    }

    MultiEffectBoosterVaccine::MultiEffectBoosterVaccine() 
    : MultiEffectVaccine()
    , prime_acquire( 0.0f )
    , prime_transmit( 0.0f )
    , prime_mortality( 0.0f )
    , boost_acquire( 0.0f )
    , boost_transmit( 0.0f )
    , boost_mortality( 0.0f )
    , boost_threshold_acquire( 0.0f )
    , boost_threshold_transmit( 0.0f )
    , boost_threshold_mortality( 0.0f )
    {
    }

    MultiEffectBoosterVaccine::MultiEffectBoosterVaccine( const MultiEffectBoosterVaccine& master )
        : MultiEffectVaccine( master )
        , prime_acquire( master.prime_acquire )
        , prime_transmit( master.prime_transmit )
        , prime_mortality( master.prime_mortality )
        , boost_acquire( master.boost_acquire )
        , boost_transmit( master.boost_transmit )
        , boost_mortality( master.boost_mortality )
        , boost_threshold_acquire( master.boost_threshold_acquire )
        , boost_threshold_transmit( master.boost_threshold_transmit )
        , boost_threshold_mortality( master.boost_threshold_mortality )
    {
    }

    MultiEffectBoosterVaccine::~MultiEffectBoosterVaccine()
    {
    }

        bool
    MultiEffectBoosterVaccine::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool distributed =  MultiEffectVaccine::Distribute( context, pCCO );
        if( distributed )
        {
            // store ivc for apply
            LOG_DEBUG("Distributing SimpleBoosterVaccine.\n");
            if (s_OK != context->QueryInterface(GET_IID(IVaccineConsumer), (void**)&ivc) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVaccineConsumer", "IIndividualHumanInterventionsContext" );
            }

            ApplyPrimingAndBoostingEffects(context);
        }
        return distributed;
    }

    void MultiEffectBoosterVaccine::ApplyPrimingAndBoostingEffects(IIndividualHumanInterventionsContext *context)
    {
        IDrugVaccineInterventionEffects* idvie = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IDrugVaccineInterventionEffects", "IIndividualHumanInterventionsContext" );
        }
        ISusceptibilityContext* isc = context->GetParent()->GetSusceptibilityContext();

        float acquisition_immunity =  idvie->GetInterventionReducedAcquire()*  isc->getModAcquire();
        float transmission_immunity = idvie->GetInterventionReducedTransmit()* isc->getModTransmit();
        float mortality_immunity =    idvie->GetInterventionReducedMortality()*isc->getModMortality();
        
        acquire_effect->SetInitial(prime_acquire);
        transmit_effect->SetInitial(prime_transmit);
        mortality_effect->SetInitial(prime_mortality);
        if (acquisition_immunity < (1.0f - boost_threshold_acquire) )
        {
            acquire_effect->SetInitial(boost_acquire);  
        }
        if (transmission_immunity < (1.0f - boost_threshold_transmit) ) 
        {
            transmit_effect->SetInitial(boost_transmit);   
        }
        if (mortality_immunity < (1.0f - boost_threshold_mortality) ) 
        {
            mortality_effect->SetInitial(boost_mortality);   
        }
    }

    REGISTER_SERIALIZABLE(MultiEffectBoosterVaccine);

    void MultiEffectBoosterVaccine::serialize(IArchive& ar, MultiEffectBoosterVaccine* obj)
    {
        MultiEffectVaccine::serialize( ar, obj );
        MultiEffectBoosterVaccine& vaccine = *obj;
        ar.labelElement("prime_acquire")                 & vaccine.prime_acquire;
        ar.labelElement("prime_transmit")                & vaccine.prime_transmit;
        ar.labelElement("prime_mortality")               & vaccine.prime_mortality;
        ar.labelElement("boost_acquire")                 & vaccine.boost_acquire;
        ar.labelElement("boost_transmit")                & vaccine.boost_transmit;
        ar.labelElement("boost_mortality")               & vaccine.boost_mortality;
        ar.labelElement("boost_threshold_acquire")       & vaccine.boost_threshold_acquire;
        ar.labelElement("boost_threshold_transmit")      & vaccine.boost_threshold_transmit;
        ar.labelElement("boost_threshold_mortality")     & vaccine.boost_threshold_mortality;
    }
}
