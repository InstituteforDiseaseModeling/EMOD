/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Vaccine.h"
#include "SimpleBoosterVaccine.h"
#include "Common.h"                  // for INFINITE_TIME
#include "Contexts.h"                // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionsContainer.h"  // for IVaccineConsumer methods
#include "RANDOM.h"                  // for ApplyVaccineTake random draw

SETUP_LOGGING( "SimpleBoosterVaccine" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(SimpleBoosterVaccine)

    bool
    SimpleBoosterVaccine::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap( "Prime_Effect", &prime_effect, SBV_Prime_Effect_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "Boost_Effect", &boost_effect, SBV_Boost_Effect_DESC_TEXT, 0.0, 1.0, 1.0 );
        initConfigTypeMap( "Boost_Threshold", &boost_threshold, SBV_Boost_Threshold_DESC_TEXT, 0.0, 1.0, 0.0);        
        
        bool configured = SimpleVaccine::Configure( inputJson );
        return configured;
    }

    SimpleBoosterVaccine::SimpleBoosterVaccine() 
    : SimpleVaccine()
    , prime_effect(0.0)
    , boost_effect(0.0)
    , boost_threshold(0.0)
    {
    }

    SimpleBoosterVaccine::SimpleBoosterVaccine( const SimpleBoosterVaccine& master )
    : SimpleVaccine( master )
    , prime_effect(master.prime_effect)
    , boost_effect(master.boost_effect)
    , boost_threshold(master.boost_threshold)
    {
    }

    SimpleBoosterVaccine::~SimpleBoosterVaccine()
    {
    }

    bool
    SimpleBoosterVaccine::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        // store ivc for apply
        LOG_DEBUG("Distributing SimpleBoosterVaccine.\n");
        if (s_OK != context->QueryInterface(GET_IID(IVaccineConsumer), (void**)&ivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVaccineConsumer", "IIndividualHumanInterventionsContext" );
        }

        ApplyPrimingAndBoostingEffects(context);

        bool distribute = SimpleVaccine::Distribute( context, pCCO );
        return distribute;
    }

    void SimpleBoosterVaccine::ApplyPrimingAndBoostingEffects(IIndividualHumanInterventionsContext *context)
    {
        IDrugVaccineInterventionEffects* idvie = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IDrugVaccineInterventionEffects", "IIndividualHumanInterventionsContext" );
        }
        ISusceptibilityContext* isc = context->GetParent()->GetSusceptibilityContext();

        float acquisition_immunity = idvie->GetInterventionReducedAcquire()*isc->getModAcquire();
        float transmission_immunity = idvie->GetInterventionReducedTransmit()*isc->getModTransmit();
        float mortality_immunity = idvie->GetInterventionReducedMortality()*isc->getModMortality();
        LOG_DEBUG_F( "Prime Effect = %f\n", prime_effect );
        LOG_DEBUG_F( "Boost Effect = %f\n", boost_effect );

        auto effect = 0.0f;
        bool boost = false;
        if( ( (vaccine_type == SimpleVaccineType::AcquisitionBlocking) &&  (acquisition_immunity < (1.0f - boost_threshold) ) )   ||
            ( (vaccine_type == SimpleVaccineType::TransmissionBlocking) && (transmission_immunity < (1.0f - boost_threshold) )  ) ||
            ( (vaccine_type == SimpleVaccineType::MortalityBlocking) &&    (mortality_immunity < (1.0f - boost_threshold) ) )     ||
            ( (vaccine_type == SimpleVaccineType::Generic) &&              (acquisition_immunity*transmission_immunity*mortality_immunity < (1.0f - boost_threshold) ) )
          )
        {
            boost = true;
        }

        if( boost )
        {
            LOG_DEBUG_F( "Setting Current to Boost Effect: %f\n", boost_effect );
            effect = boost_effect;
        }
        else
        {
            LOG_DEBUG_F( "Setting Current to Prime Effect: %f\n", prime_effect );
            effect = prime_effect;
        }

        waning_effect->SetInitial( effect );
    }

    REGISTER_SERIALIZABLE(SimpleBoosterVaccine);

    void SimpleBoosterVaccine::serialize(IArchive& ar, SimpleBoosterVaccine* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleBoosterVaccine& vaccine = *obj;
        ar.labelElement("vaccine_type")    & vaccine.vaccine_type;
        ar.labelElement("vaccine_take")    & vaccine.vaccine_take;
        ar.labelElement("vaccine_took")    & vaccine.vaccine_took;
        ar.labelElement("prime_effect")    & vaccine.prime_effect;
        ar.labelElement("boost_effect")    & vaccine.boost_effect;
        ar.labelElement("waning_effect")   & vaccine.waning_effect;
        ar.labelElement("boost_threshold") & vaccine.boost_threshold;
    }
}
