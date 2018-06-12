/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ReportTB.h"
#include "ReportTBHIV.h" // for base class

#include "IndividualCoInfection.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"
#include "TBContexts.h"

SETUP_LOGGING( "ReportTBHIV" )

namespace Kernel 
{
    static const char * _all_TB_HIV_label= "TB HIV+ Prevalence ";
    static const char * _latent_TB_HIV_prevalence_label = "Latent TB in HIV+ Prevalence";
    static const char * _HIV_prevalence_label = "HIV prevalence";
    static const char * _active_TB_HIV_prevalence_label = "Active TB in HIV+ Prevalence";

    static const char * _active_mean_CD4_TB_HIV_label = "Mean CD4 count coinfected";
    static const char * _active_TB_CD4_500_label = "Prevalence Active TB CD4 > 500";
    static const char * _active_TB_CD4_500_350_label = "Prevalence Active TB  350< CD4< 500";
    static const char * _active_TB_CD4_less_350_label = "Prevalence Acitve TB CD4 < 350";

    static const char * _prevalence_of_ART_label = "Prevalence of ART in HIV+";
    static const char * _active_treatment_tb_label = "Number Active TB on Treatment"; 
    static const char * _new_active_TB_HIV_label = "New Active TB with HIV";
    static const char * _new_active_TB_HIV_ART_label = "New Active TB with HIV on ART";
    static const char * _new_TB_deaths_HIV_label = "New TB deaths with HIV";
    static const char * _new_TB_deaths_HIV_on_ART_label = "New TB deaths with HIV on ART";
    static const char * _tb_treatment_label = "On TB Treatment";

    ReportTBHIV::ReportTBHIV()
        :ReportTB()
    {}

    void ReportTBHIV::UpdateSEIRW(const IIndividualHuman * const individual, float monte_carlo_weight)
    {
        IIndividualHumanCoInfection* tbhiv_ind = NULL;
        if ((const_cast<IIndividualHuman*>(individual))->QueryInterface(GET_IID(IIndividualHumanCoInfection), (void**)&tbhiv_ind) != s_OK)
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanCoInfection", "IndividualHuman");
        }

        if (!individual->IsInfected())  // Susceptible, Recovered (Immune), or Waning
        {
            NonNegativeFloat acquisitionModifier = tbhiv_ind->GetImmunityReducedAcquire() * individual->GetInterventionReducedAcquire(); //Line in Report TB
            if (acquisitionModifier >= 1.0f)
            {
                countOfSusceptibles += monte_carlo_weight;
            }
            else if (acquisitionModifier > 0.0f)
            {
                countOfWaning += monte_carlo_weight;
            }
            else
            {
                countOfRecovered += monte_carlo_weight;
            }
        }
        else // Exposed or Infectious
        {
            IIndividualHumanTB* ihtb = nullptr;
            if ((const_cast<IIndividualHuman*>(individual))->QueryInterface(GET_IID(IIndividualHumanTB), (void**)&ihtb) != s_OK)
            {
                LOG_ERR_F("%s: individual->QueryInterface(IIndividualHumanTB) failed.\n", __FUNCTION__);
            }

            if ((individual->GetInfectiousness() > 0.0f)  || (ihtb && ihtb->IsExtrapulmonary()) )
            {
                countOfInfectious += monte_carlo_weight;
            }
            else
            {
                countOfExposed += monte_carlo_weight;
            }
        }
    }

    void ReportTBHIV::BeginTimestep()
    {
        ReportTB::BeginTimestep();

        m_CD4count_TB_and_HIV_pos_persons = 0.0f;
        m_TB_active_500_350 = 0.0f;
        m_TB_active_above_500 = 0.0f;
        m_TB_active_below_350 = 0.0f;

        m_allTB_HIV_persons = 0.0f;
        m_active_TB_HIV_persons = 0.0f;
        m_latent_TB_HIV_persons = 0.0f;
        m_HIV_persons = 0.0f;
        m_HIV_ART_persons = 0.0f;
        m_TB_active_on_treatment = 0.0f;
        m_new_active_TB_with_HIV = 0.0f;
        m_new_active_TB_with_HIV_and_ART = 0.0f;
        m_new_TB_deaths_with_HIV = 0.0f;
        m_new_TB_deaths_with_HIV_and_ART = 0.0f;
        m_TB_treatment_persons = 0.0f;
    }

    void ReportTBHIV::populateSummaryDataUnitsMap(std::map<std::string, std::string> &units_map)
    {
        ReportTB::populateSummaryDataUnitsMap(units_map);

        // Additional TB/HIV channels
        units_map[_all_TB_HIV_label]                     = "Infected fraction";
        units_map[_active_TB_HIV_prevalence_label]       = "Infected fraction";
        units_map[_latent_TB_HIV_prevalence_label]       = "Infected fraction";
        units_map[_HIV_prevalence_label]                 = "Infected fraction";
        units_map[_active_mean_CD4_TB_HIV_label]         = "CD4 count";
        units_map[_active_TB_CD4_500_label]              = "Fraction";
        units_map[_active_TB_CD4_500_350_label]          = "Fraction";
        units_map[_active_TB_CD4_less_350_label]         = "Fraction";
        units_map[_prevalence_of_ART_label]              = "Fraction";
        units_map[_active_treatment_tb_label]            = "Number";
        units_map[_new_active_TB_HIV_ART_label]          = "Number";
        units_map[_new_active_TB_HIV_label]              = "Number";
        units_map[_new_TB_deaths_HIV_label]              = "Number";
        units_map[_new_TB_deaths_HIV_on_ART_label]       = "Number";
        units_map[_tb_treatment_label]                   = "Number";
    }

    void ReportTBHIV::postProcessAccumulatedData()
    {
        // make sure to normalize Mean CD4 count BEFORE HIV Prevalence in TB positive
        normalizeChannel(_active_mean_CD4_TB_HIV_label, _all_TB_HIV_label);
        // Normalize TB-specific summary data channels, note do this before "Infected" and Active/Latent prevalence is normalized in ReportTB
        normalizeChannel(_all_TB_HIV_label,     "Statistical Population");
        ReportTB::postProcessAccumulatedData();
        normalizeChannel(_active_TB_CD4_500_label,      _active_TB_HIV_prevalence_label);
        normalizeChannel(_active_TB_CD4_500_350_label, _active_TB_HIV_prevalence_label);
        normalizeChannel(_active_TB_CD4_less_350_label, _active_TB_HIV_prevalence_label);
        normalizeChannel(_active_TB_HIV_prevalence_label, _HIV_prevalence_label);
        normalizeChannel(_latent_TB_HIV_prevalence_label, _HIV_prevalence_label);
        normalizeChannel(_prevalence_of_ART_label, _HIV_prevalence_label);
        normalizeChannel(_HIV_prevalence_label,        "Statistical Population"); 
    }

    void
        ReportTBHIV::LogNodeData( INodeContext * pNode )
    {
        Accumulate(_active_mean_CD4_TB_HIV_label, m_CD4count_TB_and_HIV_pos_persons );
        Accumulate(_all_TB_HIV_label,      m_allTB_HIV_persons );
        Accumulate(_active_TB_HIV_prevalence_label,      m_active_TB_HIV_persons );
        Accumulate(_latent_TB_HIV_prevalence_label, m_latent_TB_HIV_persons );
        Accumulate( _HIV_prevalence_label,         m_HIV_persons );

        Accumulate(_active_TB_CD4_less_350_label,  m_TB_active_below_350);
        Accumulate(_active_TB_CD4_500_350_label,  m_TB_active_500_350);
        Accumulate(_active_TB_CD4_500_label,  m_TB_active_above_500);
        Accumulate(_prevalence_of_ART_label, m_HIV_ART_persons);
        Accumulate(_active_treatment_tb_label, m_TB_active_on_treatment);
        Accumulate(_new_active_TB_HIV_ART_label, m_new_active_TB_with_HIV_and_ART);
        Accumulate(_new_active_TB_HIV_label, m_new_active_TB_with_HIV);
        Accumulate(_new_TB_deaths_HIV_label, m_new_TB_deaths_with_HIV);
        Accumulate(_new_TB_deaths_HIV_on_ART_label, m_new_TB_deaths_with_HIV_and_ART);
        Accumulate(_tb_treatment_label, m_TB_treatment_persons);

        ReportTB::LogNodeData( pNode );
    }

    void ReportTBHIV::LogIndividualData( IIndividualHuman * individual )
    { 
        ReportTB::LogIndividualData(individual);
        // Cast from IndividualHuman to IndividualHumanCoinfection
        float mc_weight = (float) individual->GetMonteCarloWeight();

        IIndividualHumanCoInfection* tbhiv_ind = NULL;
        if( individual->QueryInterface( GET_IID(IIndividualHumanCoInfection), (void**) &tbhiv_ind ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanCoInfection", "IndividualHuman" );
        } 

        if(tbhiv_ind->HasHIV())
        {
            LOG_VALID_F( "Individual %d has HIV.\n", individual->GetSuid().data );
            m_HIV_persons += mc_weight;

            IIndividualHumanHIV* pHIVHum = nullptr;
            if (individual->QueryInterface(GET_IID(IIndividualHumanHIV), (void**) &pHIVHum ) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualHumanHIV", "IndividualHuman" );
            }

            if (pHIVHum->GetHIVInterventionsContainer()->OnArtQuery() == true)
            {
                m_HIV_ART_persons += mc_weight;
            } 

            if( individual->GetNewInfectionState() == NewInfectionState::NewlyActive && tbhiv_ind->HasHIV())
            {
                m_new_active_TB_with_HIV += mc_weight;

                if (pHIVHum->GetHIVInterventionsContainer()->OnArtQuery() == true)
                {
                    m_new_active_TB_with_HIV_and_ART += mc_weight;
                }
            }       

            if (individual->GetStateChange() == HumanStateChange::KilledByInfection)
            {
                m_new_TB_deaths_with_HIV += mc_weight;

                if (pHIVHum->GetHIVInterventionsContainer()->OnArtQuery() == true)
                {
                    m_new_TB_deaths_with_HIV_and_ART += mc_weight;
                }
            }        
        }


        if ( tbhiv_ind->GetTBInterventionsContainer()->GetNumTBDrugsActive() >= 1 )
        {   
            m_TB_treatment_persons += mc_weight;

            if (tbhiv_ind->HasActiveInfection() )  //hmm in  a way should redo this interface to be TB specific and not just for TBHIV

            {
                m_TB_active_on_treatment += mc_weight;
            }
        }

        float immune_strata = tbhiv_ind->GetCD4();

        if (tbhiv_ind->HasHIV() && tbhiv_ind->HasTB())
        {
            m_allTB_HIV_persons += mc_weight;
            if (tbhiv_ind->HasActiveInfection())
            {
                m_CD4count_TB_and_HIV_pos_persons += tbhiv_ind->GetCD4() * mc_weight; 
                m_active_TB_HIV_persons += mc_weight;

                if (immune_strata > 500.0)
                    m_TB_active_above_500 += mc_weight;
                else if (immune_strata > 350.0 )
                    m_TB_active_500_350 += mc_weight;
                else if (immune_strata < 350.0)
                    m_TB_active_below_350 += mc_weight;
            }
            else
            {
                m_latent_TB_HIV_persons += mc_weight; 
            }
        }
    }
}
