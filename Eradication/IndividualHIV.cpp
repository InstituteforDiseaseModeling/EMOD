/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include <typeinfo>
#include "IndividualHIV.h"
#include "InfectionHIV.h"
#include "NodeEventContext.h"
#include "SusceptibilityHIV.h"
#include "HIVInterventionsContainer.h"
#include "SimulationConfig.h"
#include "EventTrigger.h"

SETUP_LOGGING( "IndividualHIV" )

namespace Kernel
{
    #define TWELVE_WEEKS    (12*7.0f)
    #define FOURTEEN_WEEKS  (14*7.0f)
    #define SIX_WEEKS       (6*7.0f)
    #define EIGHTEEN_MONTHS (18*30.0f)


    float IndividualHumanHIVConfig::maternal_transmission_ART_multiplier = 1.0f;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(IndividualHumanHIV,IndividualHumanHIVConfig)
    BEGIN_QUERY_INTERFACE_BODY(IndividualHumanHIVConfig)
    END_QUERY_INTERFACE_BODY(IndividualHumanHIVConfig)

    bool IndividualHumanHIVConfig::Configure( const Configuration* config )
    {
        initConfigTypeMap( "Maternal_Transmission_ART_Multiplier", &maternal_transmission_ART_multiplier, Maternal_Transmission_ART_Multiplier_DESC_TEXT, 0.0f, 1.0f, 0.1f );
        IndividualHumanConfig::enable_immunity = true;
        return JsonConfigurable::Configure( config );
    }

    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanHIV, IndividualHumanSTI)
        HANDLE_INTERFACE(IIndividualHumanHIV)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanHIV, IndividualHumanSTI)

    IndividualHumanHIV *IndividualHumanHIV::CreateHuman(INodeContext *context, suids::suid id, float MCweight, float init_age, int gender)
    {
        IndividualHumanHIV *newindividual = _new_ IndividualHumanHIV(id, MCweight, init_age, gender);

        newindividual->SetContextTo(context);
        newindividual->InitializeConcurrency();
        LOG_DEBUG_F( "Created human with age=%f\n", newindividual->m_age );

        return newindividual;
    }

    void IndividualHumanHIV::InitializeHuman()
    {
        IndividualHumanSTI::InitializeHuman();
    }

    void IndividualHumanHIV::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        auto susc = SusceptibilityHIV::CreateSusceptibility(this, m_age, imm_mod, risk_mod);
        susceptibility = susc; // serialization/migration?
        if ( susc->QueryInterface(GET_IID(ISusceptibilityHIV), (void**)&hiv_susceptibility) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "susc", "IHIVSusceptibilityHIV", "Susceptibility" );
        }
    }

    IndividualHumanHIV::IndividualHumanHIV(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender)
        : IndividualHumanSTI(_suid, monte_carlo_weight, initial_age, gender)
        , pos_num_partners_while_CD4500plus(0)
        , neg_num_partners_while_CD4500plus(0)
    {
        LOG_DEBUG("created IndividualHumanHIV\n");
    }

    IndividualHumanHIV::~IndividualHumanHIV()
    {
        LOG_DEBUG_F( "%lu (HIV) destructor.\n", this->GetSuid().data );
    }

    IInfection* IndividualHumanHIV::createInfection( suids::suid _suid )
    {
        return InfectionHIV::CreateInfection(this, _suid);
    }

    void IndividualHumanHIV::InitializeStaticsHIV( const Configuration* config )
    {
        InfectionHIVConfig infection_config;
        infection_config.enable_disease_mortality = true;  //fixed on for HIV, needs to be set explicitly because SIM_HIV is not in depends-on clause (and thus not read)
        infection_config.Configure( config );
        SusceptibilityHIVConfig immunity_config;
        immunity_config.Configure( config );
        IndividualHumanHIVConfig individual_config;
        individual_config.Configure( config );
    }

    void IndividualHumanHIV::setupInterventionsContainer()
    {
        m_pSTIInterventionsContainer = _new_ HIVInterventionsContainer();
        interventions = m_pSTIInterventionsContainer;
    }

    bool IndividualHumanHIV::HasHIV() const
    {
        bool ret = false;
        for (auto infection : infections)
        {
            IInfectionHIV* pinfHIV = nullptr;
            if (s_OK == infection->QueryInterface(GET_IID( IInfectionHIV ), (void**)&pinfHIV) )
            {
                ret = true;
                break;
            }
        }
        return ret;
    }

    IInfectionHIV *
    IndividualHumanHIV::GetHIVInfection()
    const
    {
        IInfectionHIV* pinfHIV = nullptr;
        if( infections.size() == 0 )
        {
            return nullptr;
        }
        else if (s_OK == (*infections.begin())->QueryInterface(GET_IID( IInfectionHIV ), (void**)&pinfHIV) )
        {
            return pinfHIV;
        }
        else
        {
            // exception?
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "(*infections.begin())", "IInfectionHIV", "Infection" );
        }
    }

    ISusceptibilityHIV*
    IndividualHumanHIV::GetHIVSusceptibility()
    const
    {
        return hiv_susceptibility;
    }

    IHIVInterventionsContainer*
    IndividualHumanHIV::GetHIVInterventionsContainer()
    const
    {
        IHIVInterventionsContainer *ic = nullptr;
        if (s_OK == interventions->QueryInterface(GET_IID( IHIVInterventionsContainer ), (void**)&ic) )
        {
            return ic;
        } else {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "interventions", "IHIVInterventionsContainer", "interventions" );
        }

    }

    void
    IndividualHumanHIV::Update( float curtime, float dt )
    {
        IndividualHumanSTI::Update( curtime, dt );

        if (IndividualHumanConfig::aging)
        {
            if( ((m_age - dt) < SIX_WEEKS) && (SIX_WEEKS <= m_age) )
            {
                broadcaster->TriggerObservers( GetEventContext(), EventTrigger::SixWeeksOld );
            }
            else if( ((m_age - dt) < EIGHTEEN_MONTHS) && (EIGHTEEN_MONTHS <= m_age) )
            {
                broadcaster->TriggerObservers( GetEventContext(), EventTrigger::EighteenMonthsOld );
            }
        }
    }

    bool IndividualHumanHIV::UpdatePregnancy(float dt)
    {
        bool birth_this_timestep = IndividualHumanSTI::UpdatePregnancy( dt );
        
        if( is_pregnant )
        {
            if( ((pregnancy_timer - dt) < (DAYSPERWEEK*WEEKS_FOR_GESTATION - TWELVE_WEEKS)) && 
                                          ((DAYSPERWEEK*WEEKS_FOR_GESTATION - TWELVE_WEEKS) <= pregnancy_timer) )
            {
                LOG_DEBUG_F( "Hit 12 weeks in pregnancy with pregnancy_timer of %f\n", pregnancy_timer );
                broadcaster->TriggerObservers( GetEventContext(), EventTrigger::TwelveWeeksPregnant);
            }

            if( ((pregnancy_timer - dt) < (DAYSPERWEEK*WEEKS_FOR_GESTATION - FOURTEEN_WEEKS)) && 
                                          ((DAYSPERWEEK*WEEKS_FOR_GESTATION - FOURTEEN_WEEKS) <= pregnancy_timer) )
            {
                LOG_DEBUG_F( "Hit 14 weeks in pregnancy with pregnancy_timer of %f\n", pregnancy_timer );
                broadcaster->TriggerObservers( GetEventContext(), EventTrigger::FourteenWeeksPregnant );
            }
            else
            {
                LOG_DEBUG_F( "pregnancy_timer = %f\n", pregnancy_timer );
            }
        }

        return birth_this_timestep;
    }

    ProbabilityNumber
    IndividualHumanHIV::getProbMaternalTransmission()
    const
    {
        ProbabilityNumber retValue = IndividualHuman::getProbMaternalTransmission();
        auto mod = float(GetHIVInterventionsContainer()->GetProbMaternalTransmissionModifier());
        if( GetHIVInterventionsContainer()->OnArtQuery() && GetHIVInterventionsContainer()->GetArtStatus() != ARTStatus::ON_BUT_ADHERENCE_POOR )
        {
            retValue *= IndividualHumanHIVConfig::maternal_transmission_ART_multiplier;
            LOG_DEBUG_F( "Mother giving birth on ART: prob tx = %f\n", float(retValue) );
        }
        else if( mod > 0 )
        {
            // 100% "modifier" = 0% prob of transmission.
            retValue *= (1.0f - mod );
            LOG_DEBUG_F( "Mother giving birth on PMTCT: prob tx = %f\n", float(retValue) );
        }
        return retValue;
    }


    std::string
    IndividualHumanHIV::toString()
    const
    {
        return IndividualHumanSTI::toString();
#if 0
        std::ostringstream me;
        me << "id="
           << GetSuid().data
           << ",gender="
           << ( GetGender()==MALE ? "male" : "female" )
           << ",age="
           << GetAge()/DAYSPERYEAR
           << ",num_infections="
           << infections.size()
           << ",num_relationships="
           << relationships.size()
           << ",num_relationships_lifetime="
           << num_lifetime_relationships
           << ",num_relationships_last_6_months="
           << last_6_month_relationships.size()
           << ",promiscuity_flags="
           << std::hex << static_cast<unsigned>(promiscuity_flags)
           ;
        return me.str();
#endif
    }

    REGISTER_SERIALIZABLE(IndividualHumanHIV);

    void IndividualHumanHIV::serialize(IArchive& ar, IndividualHumanHIV* obj)
    {
        IndividualHumanSTI::serialize( ar, obj );
        IndividualHumanHIV& ind_hiv = *obj;
        ar.labelElement("has_active_TB"                     ) & ind_hiv.has_active_TB;
        ar.labelElement("pos_num_partners_while_CD4500plus" ) & ind_hiv.pos_num_partners_while_CD4500plus;
        ar.labelElement("neg_num_partners_while_CD4500plus" ) & ind_hiv.neg_num_partners_while_CD4500plus;

        if( ar.IsReader() )
        {
            if ( ind_hiv.susceptibility->QueryInterface(GET_IID(ISusceptibilityHIV), (void**)&ind_hiv.hiv_susceptibility) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "susc", "IHIVSusceptibilityHIV", "Susceptibility" );
            }
        }
    }
}
