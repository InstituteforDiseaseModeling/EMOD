/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "ReportTB.h" // for base class
#include "IndividualTB.h"
#include "NodeTB.h" //for lognodedata
#include "Types.h"
static const char* _module = "ReportTB";



namespace Kernel {

static const char * _latent_TB_label = "Latent TB Prevalence";
static const char * _latent_fast_label = "Latent Fast TB Prevalence";
static const char * _active_TB_label = "Active TB Prevalence";
static const char * _active_presymp_label = "Active Presymptomatic Prevalence";
static const char * _active_smear_neg_label = "Active Smear neg Prevalence";
static const char * _sx_smear_pos_label = "Active Sx Smear pos Prevalence";
static const char * _sx_smear_neg_label = "Active Sx Smear neg Prevalence";
static const char * _sx_smear_extrapulm_label = "Active Sx Extrapulm Prevalence";
static const char * _mdr_TB_label = "MDR TB Prevalence";
static const char * _active_mdr_TB_label = "Active MDR TB Prevalence";
static const char * _TB_immune_fraction_label = "TB Immune Fraction";

static const char * _new_active_label = "New Active TB Infections";
static const char * _newly_cleared_label = "Newly Cleared TB Infections";
static const char * _new_smear_pos_label = "New Smear Positive Infections";
static const char * _new_active_fast_label = "New Active Fast TB Infections";
static const char * _new_active_slow_label = "New Active Slow TB Infections";

static const char * _active_sx_label = "Active Symptomatic Prevalence";
static const char * _mdr_active_sx_label = "Fraction Sx that is MDR";
static const char * _mdr_active_sx_smear_pos_label = "Fraction Smear pos that is MDR";
static const char * _mdr_active_sx_evolved_label = "Fraction Sx MDR that is evolved";
static const char * _infectiousness_fast_label = "Infectiousness Fast Progressors";

static const char * _disease_deaths_MDR_label = "Disease Deaths MDR";

ReportTB::ReportTB()
:ReportAirborne()
{
    disease_deaths_MDR = 0.0f;
}

void ReportTB::BeginTimestep()
{
    Report::BeginTimestep();

    latent_TB_persons = 0.0f;
    latent_fast = 0.0f;
    active_TB_persons = 0.0f;
    active_presymptomatic = 0.0f;
    active_smear_neg = 0.0f;
    active_sx_smear_pos = 0.0f;
    active_sx_smear_neg = 0.0f;
    active_sx_extrapulm = 0.0f;

    MDR_TB_persons = 0.0f;
    active_MDR_TB_persons = 0.0f;
    TB_immune_persons = 0.0f;

    new_active_TB_infections = 0.0f;
    newly_cleared_TB_infections = 0.0f;
    new_smear_positive_infections = 0.0f;
    new_active_fast_TB_infections = 0.0f;
    new_active_slow_TB_infections = 0.0f;

    active_sx = 0.0f;
    mdr_active_sx = 0.0f;;
    mdr_active_sx_smear_pos = 0.0f;
    mdr_active_sx_evolved = 0.0f;

    infectiousness_fast = 0.0f;

}

void
ReportTB::populateSummaryDataUnitsMap(
    std::map<std::string, std::string> &units_map
)
{
    ReportAirborne::populateSummaryDataUnitsMap(units_map);
    
    // Additional TB channels
    units_map[_latent_TB_label]                = _infected_fraction_label;
    units_map[_latent_fast_label]          = _infected_fraction_label;
    units_map[_active_TB_label]                = _infected_fraction_label;
    units_map[_active_presymp_label]        = _infected_fraction_label;
    units_map[_active_smear_neg_label]      = _infected_fraction_label;
    units_map[_sx_smear_pos_label]            = _infected_fraction_label;
    units_map[_sx_smear_neg_label]            = _infected_fraction_label;
    units_map[_sx_smear_extrapulm_label]    = _infected_fraction_label;
    units_map[_mdr_TB_label]                = _infected_fraction_label;
    units_map[_active_mdr_TB_label]            = _infected_fraction_label;
    units_map[_TB_immune_fraction_label]            = "Immune fraction";

    units_map[_new_active_label]          = "";
    units_map[_newly_cleared_label]       = "";
    units_map[_new_smear_pos_label]       = "";

    units_map[_new_active_fast_label]  = "";
    units_map[_new_active_slow_label]  = "";

    units_map[_active_sx_label]          = "";
    units_map[_mdr_active_sx_label]       = "MDR fraction";
    units_map[_mdr_active_sx_smear_pos_label]       = "MDR fraction";
    units_map[_mdr_active_sx_evolved_label]          = "MDR fraction";

    units_map[ _infectiousness_fast_label] = "";

    units_map[_disease_deaths_MDR_label] = "";
}

void
ReportTB::postProcessAccumulatedData()
{
    ReportAirborne::postProcessAccumulatedData();

    // Normalize TB-specific summary data channels
    normalizeChannel(_latent_TB_label,            _stat_pop_label);
    normalizeChannel(_latent_fast_label,        _stat_pop_label);
    normalizeChannel(_active_TB_label,            _stat_pop_label);
    normalizeChannel(_mdr_TB_label,                _stat_pop_label);
    normalizeChannel(_active_mdr_TB_label,        _stat_pop_label);
    normalizeChannel(_TB_immune_fraction_label, _stat_pop_label);

    normalizeChannel(_mdr_active_sx_evolved_label,        _mdr_active_sx_label);
    normalizeChannel(_mdr_active_sx_label,                _active_sx_label);
    normalizeChannel(_mdr_active_sx_smear_pos_label,    _sx_smear_pos_label);

    normalizeChannel(_active_presymp_label,        _stat_pop_label);
    normalizeChannel(_sx_smear_pos_label,        _stat_pop_label);
    normalizeChannel(_sx_smear_neg_label,        _stat_pop_label);
    normalizeChannel(_sx_smear_extrapulm_label, _stat_pop_label);
    normalizeChannel(_active_smear_neg_label,   _stat_pop_label);
    normalizeChannel(_active_sx_label,            _stat_pop_label);
}

void ReportTB::UpdateSEIRW( const IndividualHuman * const individual, float monte_carlo_weight )
{
    if (!individual->IsInfected())  // Susceptible, Recovered (Immune), or Waning
    {
        NonNegativeFloat acquisitionModifier = individual->GetImmunityReducedAcquire() * individual->GetInterventionReducedAcquire();
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
        IIndividualHumanTB2* ihtbtoo = nullptr;
        if ((const_cast<IndividualHuman*>(individual))->QueryInterface(GET_IID(IIndividualHumanTB2), (void**)&ihtbtoo) != s_OK)
        {
            LOG_ERR_F("%s: individual->QueryInterface(IIndividualHumanTB2) failed.\n", __FUNCTION__);
        }

        if ((individual->GetInfectiousness() > 0.0f) || (ihtbtoo && ihtbtoo->IsExtrapulmonary()))
        {
            countOfInfectious += monte_carlo_weight;
        }
        else
        {
            countOfExposed += monte_carlo_weight;
        }
    }
}

void
ReportTB::LogIndividualData(
    Kernel::IndividualHuman * individual
)
{
    ReportAirborne::LogIndividualData( individual );

    float monte_carlo_weight = (float)individual->GetMonteCarloWeight();
    const IndividualHumanTB* individual_tb = static_cast<const IndividualHumanTB*>(individual);

    //Immunity
    if (individual_tb->IsImmune() && !individual->IsInfected())
    {
        TB_immune_persons += monte_carlo_weight;
    }

    //deaths by MDR
    if(individual->GetStateChange() == HumanStateChange::KilledByInfection)
    {
        if (individual_tb->IsMDR() )
        {
            disease_deaths_MDR += monte_carlo_weight;
        }
    }

    // newinfectionstate
    if( individual->GetNewInfectionState() == NewInfectionState::NewlyActive)
    {
        new_active_TB_infections +=  monte_carlo_weight;

        if (individual_tb->IsFastProgressor())
        {
            new_active_fast_TB_infections += monte_carlo_weight;
        }    
        else  
        {
            new_active_slow_TB_infections += monte_carlo_weight;
        }


        if (individual_tb->IsSmearPositive())
        {
            new_smear_positive_infections += monte_carlo_weight;
        }        
    }
    if( individual->GetNewInfectionState() == NewInfectionState::NewlyCleared)
    {
        newly_cleared_TB_infections += monte_carlo_weight;
    }   

    //Latent people
    if ( individual_tb->HasLatentInfection() ) 
    {
        latent_TB_persons += monte_carlo_weight;

        if (individual_tb->IsFastProgressor() )
        {
            latent_fast += monte_carlo_weight;
        }
    }

    //Active people
    if ( individual_tb->HasActiveInfection() )
    {
        active_TB_persons += monte_carlo_weight;
        if ( individual_tb->IsFastProgressor() ) 
        {
            infectiousness_fast += ( individual->GetInfectiousness() * monte_carlo_weight );
        }

        //active mdr people
        if (individual_tb->IsMDR() )
        {
            active_MDR_TB_persons += monte_carlo_weight;
        }

        //Active by stage and smear status
        if ( individual_tb->HasActivePresymptomaticInfection() )
        {
            active_presymptomatic += monte_carlo_weight;

            if (!individual_tb->IsSmearPositive() && !individual_tb->IsExtrapulmonary() )
            {
                active_smear_neg += monte_carlo_weight;
            }
        }
        else
        {
            active_sx += monte_carlo_weight;

            if ( individual_tb->IsSmearPositive() )
            {
                active_sx_smear_pos += monte_carlo_weight;
                if ( individual_tb->IsMDR() )
                {
                    mdr_active_sx_smear_pos += monte_carlo_weight;
                }
            }
            else if ( individual_tb->IsExtrapulmonary() )
            {
                active_sx_extrapulm += monte_carlo_weight;
            }
            else
            {
                active_sx_smear_neg += monte_carlo_weight;
                active_smear_neg += monte_carlo_weight;
            }

            if (individual_tb->IsMDR() ) 
            { 
                mdr_active_sx += monte_carlo_weight; 
                if (individual_tb->IsEvolvedMDR() )
                {
                    mdr_active_sx_evolved += monte_carlo_weight;
                }
            }
        }
    }

    //MDR both latent and active
    if ( individual_tb->IsMDR() )
    {
        MDR_TB_persons += monte_carlo_weight;
    }

}

void
ReportTB::LogNodeData(
    Kernel::INodeContext * pNC
)
{
    ReportAirborne::LogNodeData( pNC );

    Accumulate(_latent_TB_label,                latent_TB_persons );
    Accumulate(_latent_fast_label,                latent_fast);
    Accumulate(_active_TB_label,                active_TB_persons );
    Accumulate(_active_presymp_label,            active_presymptomatic);
    Accumulate(_active_smear_neg_label,            active_smear_neg );
    Accumulate(_sx_smear_pos_label,                active_sx_smear_pos );
    Accumulate(_sx_smear_neg_label,                active_sx_smear_neg );
    Accumulate(_sx_smear_extrapulm_label,        active_sx_extrapulm );
    Accumulate(_active_sx_label,                active_sx);
    
    Accumulate(_mdr_TB_label,                    MDR_TB_persons );
    Accumulate(_active_mdr_TB_label,            active_MDR_TB_persons );
    Accumulate(_TB_immune_fraction_label,        TB_immune_persons );

    Accumulate(_new_active_label,                new_active_TB_infections );
    Accumulate(_newly_cleared_label,            newly_cleared_TB_infections );
    Accumulate(_new_smear_pos_label,            new_smear_positive_infections );
    Accumulate(_new_active_fast_label,            new_active_fast_TB_infections );
    Accumulate(_new_active_slow_label,            new_active_slow_TB_infections );

    Accumulate(_mdr_active_sx_evolved_label,        mdr_active_sx_evolved );
    Accumulate(_mdr_active_sx_label,                mdr_active_sx );
    Accumulate(_mdr_active_sx_smear_pos_label,        mdr_active_sx_smear_pos );

    Accumulate(_infectiousness_fast_label,            infectiousness_fast );

    Accumulate(_disease_deaths_MDR_label,            disease_deaths_MDR );

    const INodeTB* pTBNode = NULL;
    if( pNC->QueryInterface( GET_IID(INodeTB), (void**)&pTBNode ) != s_OK )
    {
        throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeTB", "INodeContext" );
    }
    Accumulate("New MDR active infections", pTBNode->GetMDRIncidentCounter() );
    Accumulate("New MDR evolved active infections", pTBNode->GetMDREvolvedIncidentCounter() );
    Accumulate("New MDR fast active infections", pTBNode->GetMDRFastIncidentCounter() );
    
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(ReportTB)
template<class Archive>
void serialize(Archive &ar, ReportTB& report, const unsigned int v)
{
    boost::serialization::void_cast_register<ReportTB,IReport>();
    ar &boost::serialization::base_object<Report>(report);
}
#endif

}

#endif // ENABLE_TB
