/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "SusceptibilityTB.h"
#include "Common.h"
#include "RANDOM.h"
#ifdef ENABLE_TBHIV
#include "IndividualCoinfection.h"
#else
#include "Individual.h"
#endif

static const char* _module = "SusceptibilityTB";

namespace Kernel
{
    float SusceptibilityTBConfig::TB_immune_loss_fraction = 0.0f;
    float SusceptibilityTBConfig::TB_fast_progressor_fraction_child = 0.0f;
    float SusceptibilityTBConfig::TB_fast_progressor_fraction_adult = 0.0f;
    float SusceptibilityTBConfig::TB_smear_positive_fraction_child = 0.0f;
    float SusceptibilityTBConfig::TB_smear_positive_fraction_adult = 0.0f;
    float SusceptibilityTBConfig::TB_extrapulmonary_fraction_child = 0.0f;
    float SusceptibilityTBConfig::TB_extrapulmonary_fraction_adult = 0.0f;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(SusceptibilityTBConfig,SusceptibilityTBConfig)
    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityTBConfig)
        HANDLE_INTERFACE(SusceptibilityTBConfig)
    END_QUERY_INTERFACE_BODY(SusceptibilityTBConfig)

    bool
    SusceptibilityTBConfig::Configure(
        const Configuration* config
    )
    {
        initConfigTypeMap( "TB_Immune_Loss_Fraction", &TB_immune_loss_fraction, TB_Immune_Loss_Fraction_DESC_TEXT, 0.0f, 1.0f, 0.5f );
        initConfigTypeMap( "TB_Fast_Progressor_Fraction_Child", &TB_fast_progressor_fraction_child, TB_Fast_Progressor_Fraction_Child_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "TB_Fast_Progressor_Fraction_Adult", &TB_fast_progressor_fraction_adult, TB_Fast_Progressor_Fraction_Adult_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "TB_Smear_Positive_Fraction_Child", &TB_smear_positive_fraction_child, TB_Smear_Positive_Fraction_Child_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "TB_Smear_Positive_Fraction_Adult", &TB_smear_positive_fraction_adult, TB_Smear_Positive_Fraction_Adult_DESC_TEXT, 0.0f, 1.0f, 1.0f );
        initConfigTypeMap( "TB_Extrapulmonary_Fraction_Child", &TB_extrapulmonary_fraction_child, TB_Extrapulmonary_Fraction_Child_DESC_TEXT, 0.0f, 1.0f, 0.0f );
        initConfigTypeMap( "TB_Extrapulmonary_Fraction_Adult", &TB_extrapulmonary_fraction_adult, TB_Extrapulmonary_Fraction_Adult_DESC_TEXT, 0.0f, 1.0f, 0.0f );
        auto ret = JsonConfigurable::Configure( config );

        if (TB_extrapulmonary_fraction_adult + TB_smear_positive_fraction_adult > 1.0f)
        {
           throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Fraction adult smear positive individuals is" , TB_smear_positive_fraction_adult, "Fraction adult extrapulmonary individuals", TB_extrapulmonary_fraction_adult);
        }

        if (TB_extrapulmonary_fraction_child +TB_smear_positive_fraction_child > 1.0f)
        {
            throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Fraction child smear positive individuals is" , TB_smear_positive_fraction_child, "Fraction child extrapulmonary individuals", TB_extrapulmonary_fraction_child);
        }

        LOG_INFO_F("Fraction adult smear-negative individuals is %f\n",1.0f - (TB_smear_positive_fraction_adult +TB_extrapulmonary_fraction_adult) );
        LOG_INFO_F("Fraction child smear-negative individuals is %f\n",1.0f - (TB_smear_positive_fraction_child +TB_extrapulmonary_fraction_child) );

        return ret;
    }

    BEGIN_QUERY_INTERFACE_BODY(SusceptibilityTB)
        HANDLE_INTERFACE(ISusceptibilityTB)
    END_QUERY_INTERFACE_BODY(SusceptibilityTB)


    //---------------------------- SusceptibilityTB ----------------------------------------
    SusceptibilityTB *SusceptibilityTB::CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod)
    {
        SusceptibilityTB *newsusceptibility = _new_ SusceptibilityTB(context);
        newsusceptibility->Initialize(age, immmod, riskmod);

        return newsusceptibility;
    }

    void SusceptibilityTB::Update(float dt)
    {
        age += dt; // tracks age for immune purposes

        // Immune-incompetent individuals without any current infections can lose protective immunity (as a step function) after the offset time
        // Only acquisition immunity is modeled
        // Mortality and transmit immunity not modeled
        if(!m_is_immune_competent && m_current_infections == 0 )
        {
            LOG_DEBUG_F("Acqdecayoffset = %f \n", acqdecayoffset);
            if (mod_acquire < 1) { acqdecayoffset -= dt; }
            if (SusceptibilityConfig::immune_decay && acqdecayoffset < 0 && randgen->e() < SusceptibilityConfig::acqdecayrate * dt)
            {
                m_is_immune = false; // the TB immune flag (used for reporting only) represents acquisition immunity since this is basically what BCG gives you
                mod_acquire = 1.0;            
            }
        }
    }

    void SusceptibilityTB::UpdateInfectionCleared()
    {
        if (m_current_infections > 0)
        {
            m_current_infections--;
        }
        acqdecayoffset = SusceptibilityConfig::baseacqoffset;
    }

    float SusceptibilityTB::GetFastProgressorFraction() 
    {
        float fast_fraction = 0;

        float age_years = age / DAYSPERYEAR ;
        if( !IndividualHumanConfig::IsAdultAge( age_years ) )
            fast_fraction = SusceptibilityTBConfig::TB_fast_progressor_fraction_child;
        else
            fast_fraction = SusceptibilityTBConfig::TB_fast_progressor_fraction_adult;

        return fast_fraction;
    }

    float SusceptibilityTB::GetSmearPositiveFraction() 
    {
        float smear_positive_fraction = 0;

        float age_years = age / DAYSPERYEAR ;
        if( !IndividualHumanConfig::IsAdultAge( age_years ) )
            smear_positive_fraction = SusceptibilityTBConfig::TB_smear_positive_fraction_child;
        else
            smear_positive_fraction = SusceptibilityTBConfig::TB_smear_positive_fraction_adult;

        return smear_positive_fraction;
    }

    float SusceptibilityTB::GetExtraPulmonaryFraction() 
    {
        float extrapulmonary_fraction = 0;

        float age_years = age / DAYSPERYEAR ;
        if( !IndividualHumanConfig::IsAdultAge( age_years ) )
            extrapulmonary_fraction = SusceptibilityTBConfig::TB_extrapulmonary_fraction_child;
        else
            extrapulmonary_fraction = SusceptibilityTBConfig::TB_extrapulmonary_fraction_adult;

        return extrapulmonary_fraction;
    }

    float SusceptibilityTB::GetCoughInfectiousness()
    {
        return m_cough_infectiousness;
    }

    void SusceptibilityTB::InitNewInfection()
    {
        // Individuals are given some protective immunity upon infection
        // TODO: do we want to ignore this if the flag IMMUNITY is set to 0?  The individual will not be updating it in that case.
        m_is_immune = true;
        mod_acquire = SusceptibilityConfig::baseacqupdate;
        m_current_infections++;
    }

    void SusceptibilityTB::Initialize(float _age, float _immmod, float _riskmod)
    {
        SusceptibilityAirborne::Initialize(_age, _immmod, _riskmod);
        // TODO: can later add functionality that initializes prior immune exposure based on _immmod argument
        // and node-based risk factors based on _riskmod argument
        m_is_immune = false; 
        m_current_infections = 0;
        m_is_immune_competent = randgen->e() > SusceptibilityTBConfig::TB_immune_loss_fraction;
        Flag_use_CD4_for_act = false;
        //GHH note the above does nothing with riskmod! 
        //blatant misuse of riskmod here, use as a modulator of infectiousness
        //so high riskmod = lots of uncovered coughing with lots of bacteria in your sputum = higher infectiousness 
        //you can see this in IndividualTB where infectiousness is added up by calling infection->GetInfectiousness (just does the first infection),
        //in the infectionTB when we InitializeActiveInfection, the infectiousness is calculated, so there we will call for riskmod and use as a multiplier.
        //note this is sort of similar to the m_individual_biting_risk stuff that Edward and I put together for malaria

        m_cough_infectiousness = _riskmod;
    }

    SusceptibilityTB::~SusceptibilityTB(void) { }

    bool
    SusceptibilityTB::IsImmune()
    const
    {
        return m_is_immune;
    }

    SusceptibilityTB::SusceptibilityTB()
    {
    }

    bool SusceptibilityTB::GetCD4ActFlag() const
    {
        return Flag_use_CD4_for_act;
    }
        
    void SusceptibilityTB::SetCD4ActFlag(bool bin)
    {
        Flag_use_CD4_for_act =  bin;
    }
         
    SusceptibilityTB::SusceptibilityTB(IIndividualHumanContext *context) : SusceptibilityAirborne(context) { }

    REGISTER_SERIALIZABLE(SusceptibilityTB);

    void SusceptibilityTB::serialize(IArchive& ar, SusceptibilityTB* obj)
    {
        SusceptibilityAirborne::serialize(ar, obj);
        SusceptibilityTB& susceptibility = *obj;
        ar.labelElement("Flag_use_CD4_for_act") & susceptibility.Flag_use_CD4_for_act;  // Boost serialization didn't include this member.
        ar.labelElement("m_is_immune_competent") & susceptibility.m_is_immune_competent;
        ar.labelElement("m_is_immune") & susceptibility.m_is_immune;
        ar.labelElement("m_current_infections") & susceptibility.m_current_infections;
        ar.labelElement("m_cough_infectiousness") & susceptibility.m_cough_infectiousness;
    }
}

#endif // ENABLE_TB
