/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "Debug.h" // for base class
#include "ReportSTI.h" // for base class
#include "NodeSTI.h" // for base class
#include "IRelationship.h"

static const char* _module = "ReportSTI";

//    #define MIN_SEXUAL_AGE_IN_YRS 13

namespace Kernel {

static const char* _sexually_active_prevalence_label = "Prevalence among Sexually Active (Adults)";
static const char* _sexually_active_universe_label = "Number of Individuals Ever in a Relationship";

static const char* _predebut_label      = "Pre-debut Individuals";
static const char* _single_men_label    = "Single Post-Debut Men";
static const char* _single_women_label  = "Single Post-Debut Women";
static const char* _paired_people_label = "Paired People";

static const char* _active_transitory = "Active Transitory Relationships";
static const char* _active_informal   = "Active Informal Relationships";
static const char* _active_marital    = "Active Marital Relationships";

static const char* _ymi = "Prevalence (Males, 15-49)";
static const char* _yfi = "Prevalence (Females, 15-49)";
static const char* _pdi = "Post-Debut Population";
static const char* _num_circ_males_label = "Number of Circumcised Males" ;

static NaturalNumber num_adults_not_related = 0;

    ReportSTI::ReportSTI()
        : num_marrieds(0)
        , num_singles(0)
        , sexually_mature_pop(0)
        , sexually_active_adults(0)
        , num_concordant_pos_rels(0)
        , num_concordant_neg_rels(0)
        , num_sexually_active_universe(0)
        , num_predebut(0)
        , num_single_men(0)
        , num_single_women(0)
        , num_paired(0)
        , num_transitory(0)
        , num_informal(0)
        , num_marital(0)
        , num_sexually_active_prevalance(0)
        , num_post_debut_pop(0)
        , num_circumcised_males(0)
    {
        //std::cout << "ReportSTI created." << std::endl;
    }

    void
    ReportSTI::populateSummaryDataUnitsMap(
        std::map<std::string, std::string> &units_map
    )
    {
        Report::populateSummaryDataUnitsMap(units_map);

        // Additional channels
        units_map[ _sexually_active_prevalence_label ]       = "";

        units_map[ _predebut_label      ] = "People";
        units_map[ _single_men_label    ] = "People";
        units_map[ _single_women_label  ] = "People";
        units_map[ _paired_people_label ] = "People";

        units_map[ _active_transitory ] = "Relationships";
        units_map[ _active_informal   ] = "Relationships";
        units_map[ _active_marital    ] = "Relationships";
    }

    void
    ReportSTI::LogNodeData(
        INodeContext* pNC
    )
    {
        Report::LogNodeData( pNC );

        const INodeSTI* pSTINode = NULL;
        if( pNC->QueryInterface( GET_IID(INodeSTI), (void**)&pSTINode ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeSTI", "INodeContext" );
        }
        Accumulate( _sexually_active_universe_label, num_sexually_active_universe );
    }

static NonNegativeFloat youngMaleInfected = 0.0f;
static NonNegativeFloat youngMaleDenom = 0.0f;
static NonNegativeFloat youngFemaleInfected = 0.0f;
static NonNegativeFloat youngFemaleDenom = 0.0f;

    void
    ReportSTI::LogIndividualData(
        IndividualHuman* individual
    )
    {
        Report::LogIndividualData( individual );
        IIndividualHumanSTI* sti_individual = NULL;
        if( individual->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&sti_individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndividualSTI", "IndividualHuman" );
        }

        // STI sims are never going to have non-unity Monte Carlo Weights because of the pair-forming
        unsigned int mcw = (unsigned int)individual->GetMonteCarloWeight();

        if( individual->GetAge() < 50*DAYSPERYEAR && 
            individual->GetAge() >= 15*DAYSPERYEAR

          )
        {
            if( individual->GetGender() == Gender::MALE )
            {
                youngMaleDenom += mcw;
                if( individual->IsInfected() )
                {
                    youngMaleInfected += mcw;
                }
            }
            else
            {
                youngFemaleDenom += mcw;
                if( individual->IsInfected() )
                {
                    youngFemaleInfected += mcw;
                }
            }
        }

        if( sti_individual->GetLifetimeRelationshipCount() > 0 )
        {
            num_sexually_active_universe += mcw;
            if( individual->IsInfected() )
            {
                num_sexually_active_prevalance += mcw ;
            }
        } 

        auto& relationships = sti_individual->GetRelationships();
        if (individual->GetAge() < sti_individual->GetDebutAge())
        {
            num_predebut++;
            release_assert( relationships.size() == 0 );
        }
        else
        {
            num_post_debut_pop += mcw ;
            if (relationships.size() > 0)
            {
                num_paired++;
                for (auto relationship : relationships)
                {
                    switch (relationship->GetType())
                    {
                    case RelationshipType::TRANSITORY:
                        num_transitory++;
                        break;

                    case RelationshipType::INFORMAL:
                        num_informal++;
                        break;

                    case RelationshipType::MARITAL:
                        num_marital++;
                        break;

                    default:
                        release_assert( false );
                    }
                }
            }
            else
            {
                if (individual->GetGender() == Gender::MALE)
                {
                    num_single_men++;
                }
                else
                {
                    num_single_women++;
                }
            }
        }

        if( sti_individual->IsCircumcised() )
        {
            num_circumcised_males += mcw;
        }
    }

    void ReportSTI::EndTimestep( float currentTime, float dt )
    {
        Report::EndTimestep( currentTime, dt );

        Accumulate( _single_men_label,    num_single_men );
        Accumulate( _single_women_label,  num_single_women );
        Accumulate( _paired_people_label, num_paired );
        Accumulate( _active_transitory,   num_transitory );
        Accumulate( _active_informal,     num_informal );
        Accumulate( _active_marital,      num_marital );
        Accumulate( _ymi, youngMaleInfected/youngMaleDenom );
        Accumulate( _num_circ_males_label, num_circumcised_males );

        youngMaleInfected = 0;
        youngMaleDenom = 0;
        Accumulate( _yfi, youngFemaleInfected/youngFemaleDenom );
        youngFemaleInfected = 0;
        youngFemaleDenom = 0;

        Accumulate( _sexually_active_prevalence_label, num_sexually_active_prevalance ); 
        Accumulate( _pdi, num_post_debut_pop );
        num_sexually_active_prevalance = 0 ;
        num_post_debut_pop = 0 ;
        num_predebut = num_single_men = num_single_women = num_paired = 0;
        num_transitory = num_informal = num_marital = 0;

        num_adults_not_related = 0;
        num_marrieds = 0;
        num_singles = 0;
        sexually_mature_pop = 0;
        num_concordant_pos_rels = num_concordant_neg_rels = 0;
        num_sexually_active_universe = 0;
        num_circumcised_males = 0;
    }

    void
    ReportSTI::postProcessAccumulatedData()
    {
        Report::postProcessAccumulatedData();
        normalizeChannel( _sexually_active_prevalence_label, _sexually_active_universe_label );
        channelDataMap.RemoveChannel( _new_reported_infections_label );
        channelDataMap.RemoveChannel( _cum_reported_infections_label );
        channelDataMap.RemoveChannel( _hum_infectious_res_label      );
        channelDataMap.RemoveChannel( _log_prev_label                );
        channelDataMap.RemoveChannel( _prob_new_infection_label      );
    }
}
