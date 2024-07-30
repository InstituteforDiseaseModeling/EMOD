
#include "stdafx.h"

#include "Debug.h"
#include "ReportSTI.h"
#include "IndividualSTI.h"
#include "IRelationship.h"
#include "ReportUtilitiesSTI.h"
#include "INodeContext.h"
#include "INodeSTI.h"
#include "IRelationshipManager.h"

SETUP_LOGGING( "ReportSTI" )

namespace Kernel {

    ReportSTI::ReportSTI()
        : Report()
        , m_IncludeCoitalActs( false )
        , sexually_active_prevalence_id()
        , sexually_active_universe_id()
        , single_men_id()
        , single_women_id()
        , paired_people_id()
        , ymi_id()
        , yfi_id()
        , pdi_id()
        , num_circ_males_id()
        , ymc_id()
        , yfc_id()
        , outside_pfa_id()
        , infs_outside_pfa_id()
        , active_rel_ids()
        , acts_this_dt_rel_ids()
        , acts_this_dt_condoms_rel_ids()
        , num_marrieds(0)
        , num_singles(0)
        , sexually_mature_pop(0)
        , sexually_active_adults(0)
        , num_concordant_pos_rels(0)
        , num_concordant_neg_rels(0)
        , num_sexually_active_universe(0)
        , num_single_men(0)
        , num_single_women(0)
        , num_paired(0)
        , num_outside_pfa(0)
        , num_infections_outside_pfa(0)
        , num_sexually_active_prevalance(0)
        , num_post_debut_pop(0)
        , num_circumcised_males(0)
        , youngMaleInfected(0.0f)
        , youngMaleCount(0.0f)
        , youngFemaleInfected(0.0f)
        , youngFemaleCount(0.0f)
    {
        m_EventTriggerList.push_back( EventTrigger::STINewInfection );

        sexually_active_prevalence_id = AddChannel( "Prevalence among Sexually Active"                 );
        sexually_active_universe_id   = AddChannel( "Number of Individuals Ever in a Relationship"     );
        single_men_id                 = AddChannel( "Single Post-Debut Men"                            );
        single_women_id               = AddChannel( "Single Post-Debut Women"                          );
        paired_people_id              = AddChannel( "Paired People"                                    );
        ymi_id                        = AddChannel( "Prevalence (Males, 15-49)"                        );
        yfi_id                        = AddChannel( "Prevalence (Females, 15-49)"                      );
        pdi_id                        = AddChannel( "Post-Debut Population"                            );
        num_circ_males_id             = AddChannel( "Number of Circumcised Males"                      );
        ymc_id                        = AddChannel( "Total Number of Males (15-49)"                    );
        yfc_id                        = AddChannel( "Total Number of Females (15-49)"                  );
        outside_pfa_id                = AddChannel( "Num Rels Outside PFA"                             );
        infs_outside_pfa_id           = AddChannel( "Fraction of New Infections From Rels Outside PFA" );

        for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
        {
            std::string label = std::string("Active ") + std::string(RelationshipType::pairs::get_keys()[i]) + std::string(" Relationships");
            ChannelID id = AddChannel( label );
            active_rel_ids.push_back( id );
            num_rels[ i ] = 0;
        }
    }

    bool ReportSTI::Configure( const Configuration* inputJson )
    {
        initConfigTypeMap( "Inset_Chart_Include_Coital_Acts", &m_IncludeCoitalActs, Report_STI_Include_Coital_Acts_DESC_TEXT, "Enable_Default_Reporting" );

        bool configured = Report::Configure( inputJson );
        if( configured && JsonConfigurable::_dryrun == false )
        {
        }
        return configured;
    }

    void ReportSTI::Initialize( unsigned int nrmSize )
    {
        Report::Initialize( nrmSize );
        if( m_IncludeCoitalActs )
        {
            for( int i = 0 ; i < RelationshipType::COUNT ; ++i )
            {
                std::string label2 = std::string("Coital Acts-") + std::string(RelationshipType::pairs::get_keys()[i]);
                ChannelID id2 = AddChannel( label2 );
                acts_this_dt_rel_ids.push_back( id2 );
                num_acts[ i ] = 0;

                std::string label3 = std::string("Coital Acts Using Condoms-") + std::string(RelationshipType::pairs::get_keys()[i]);
                ChannelID id3 = AddChannel( label3 );
                acts_this_dt_condoms_rel_ids.push_back( id3 );
                num_acts_condoms[ i ] = 0;
            }
        }
    }

    void
    ReportSTI::populateSummaryDataUnitsMap(
        std::map<std::string, std::string> &units_map
    )
    {
        Report::populateSummaryDataUnitsMap(units_map);

        // Additional channels
        units_map[ sexually_active_prevalence_id.GetName() ] = "";
        units_map[ sexually_active_universe_id.GetName()   ] = "";
        units_map[ single_men_id.GetName()                 ] = "People";
        units_map[ single_women_id.GetName()               ] = "People";
        units_map[ paired_people_id.GetName()              ] = "People";
        units_map[ ymi_id.GetName()                        ] = "";
        units_map[ yfi_id.GetName()                        ] = "";
        units_map[ pdi_id.GetName()                        ] = "";
        units_map[ num_circ_males_id.GetName()             ] = "";
        units_map[ ymc_id.GetName()                        ] = "";
        units_map[ yfc_id.GetName()                        ] = "";
        units_map[ outside_pfa_id.GetName()                ] = "Relationships";
        units_map[ infs_outside_pfa_id.GetName()           ] = "Fraction";

        for( auto& id : active_rel_ids )
        {
            units_map[ id.GetName() ] = "Relationships";
        }
        if( m_IncludeCoitalActs )
        {
            for( auto& id : acts_this_dt_rel_ids )
            {
                units_map[ id.GetName() ] = "Avg # Acts Per Relationship";
            }
            for( auto& id : acts_this_dt_condoms_rel_ids )
            {
                units_map[ id.GetName() ] = "Avg # Acts Per Relationship Using Condoms";
            }
        }
    }

    void
    ReportSTI::LogIndividualData(
        IIndividualHuman* individual
    )
    {
        Report::LogIndividualData( individual );

        // --------------------------------------------------------------------------------
        // --- Switched to static_cast because this method was taking 12% of total sim time
        // --- and the QueryInterface() was taking most of that. 
        // --------------------------------------------------------------------------------
        IndividualHumanSTI* sti_individual = static_cast<IndividualHumanSTI*>(individual);

        // STI sims are never going to have non-unity Monte Carlo Weights because of the pair-forming
        unsigned int mcw = (unsigned int)individual->GetMonteCarloWeight();

        if( individual->GetAge() < 50*DAYSPERYEAR && 
            individual->GetAge() >= 15*DAYSPERYEAR
          )
        {
            if( individual->GetGender() == Gender::MALE )
            {
                youngMaleCount += mcw;
                if( individual->IsInfected() )
                {
                    youngMaleInfected += mcw;
                }
            }
            else
            {
                youngFemaleCount += mcw;
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

        if (individual->GetAge() >= sti_individual->GetDebutAge())
        {
            num_post_debut_pop += mcw ;
        }

        const std::vector<IRelationship*>& relationships = sti_individual->GetRelationships();
        if (relationships.size() > 0)
        {
            num_paired++;
        }
        else if (individual->GetAge() >= sti_individual->GetDebutAge())
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

        if( sti_individual->IsCircumcised() )
        {
            num_circumcised_males += mcw;
        }
    }

    bool ReportSTI::notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
    {
        if( trigger == EventTrigger::STINewInfection )
        {
            IRelationship* p_rel = ReportUtilitiesSTI::GetTransmittingRelationship( context );
            if( p_rel == nullptr )
            {
                // no partner implies infection was result of outbreak or maternal transmission
                return true;
            }
            if( p_rel->IsOutsidePFA() )
            {
                unsigned int mcw = (unsigned int)context->GetMonteCarloWeight();
                num_infections_outside_pfa += mcw;
            }
        }
        return true;
    }

    void ReportSTI::LogNodeData( INodeContext* pNC )
    {
        Report::LogNodeData( pNC );

        INodeSTI* p_node_sti = nullptr;
        if( pNC->QueryInterface( GET_IID( INodeSTI ), (void**)&p_node_sti ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pNC", "INodeSTI", "INodeContext" );
        }

        IRelationshipManager* p_rel_mgr = p_node_sti->GetRelationshipManager();
        const tNodeRelationshipType& r_rel_id_to_rel_map = p_rel_mgr->GetNodeRelationships();

        for( auto& r_entry : r_rel_id_to_rel_map )
        {
            if( r_entry.second->GetState() == RelationshipState::NORMAL )
            {
                num_rels[ int(r_entry.second->GetType()) ]++;
                num_acts[ int(r_entry.second->GetType()) ] += r_entry.second->GetNumCoitalActs();
                num_acts_condoms[ int(r_entry.second->GetType()) ] += r_entry.second->GetNumCoitalActsUsingCondoms();
                if( r_entry.second->IsOutsidePFA() )
                {
                    this->num_outside_pfa += 1;
                }
            }
        }
    }

    void ReportSTI::EndTimestep( float currentTime, float dt )
    {
        Report::EndTimestep( currentTime, dt );

        Accumulate( single_men_id,                 num_single_men );
        Accumulate( single_women_id,               num_single_women );
        Accumulate( paired_people_id,              num_paired );
        Accumulate( outside_pfa_id,                num_outside_pfa );
        Accumulate( infs_outside_pfa_id,           num_infections_outside_pfa );
        Accumulate( ymi_id,                        youngMaleInfected );
        Accumulate( ymc_id,                        youngMaleCount );
        Accumulate( yfi_id,                        youngFemaleInfected );
        Accumulate( yfc_id,                        youngFemaleCount );
        Accumulate( num_circ_males_id,             num_circumcised_males );
        Accumulate( sexually_active_universe_id,   num_sexually_active_universe );
        Accumulate( sexually_active_prevalence_id, num_sexually_active_prevalance ); 
        Accumulate( pdi_id,                        num_post_debut_pop );

        for( int i = 0 ; i < active_rel_ids.size(); ++i )
        {
            Accumulate( active_rel_ids[ i ], num_rels[ i ] );
            num_rels[ i ] = 0;

            if( m_IncludeCoitalActs )
            {
                Accumulate( acts_this_dt_rel_ids[ i ], num_acts[ i ] );
                num_acts[ i ] = 0;

                Accumulate( acts_this_dt_condoms_rel_ids[ i ], num_acts_condoms[ i ] );
                num_acts_condoms[ i ] = 0;
            }
        }

        youngMaleInfected = 0;
        youngMaleCount = 0;
        youngFemaleInfected = 0;
        youngFemaleCount = 0;
        num_sexually_active_prevalance = 0 ;
        num_post_debut_pop = 0 ;
        num_single_men = num_single_women = num_paired = 0;
        num_outside_pfa = 0;
        num_infections_outside_pfa = 0;
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

        normalizeChannel( sexually_active_prevalence_id.GetName(), sexually_active_universe_id.GetName() );
        normalizeChannel( ymi_id.GetName(), ymc_id.GetName() );
        normalizeChannel( yfi_id.GetName(), yfc_id.GetName() );

        channelDataMap.RemoveChannel( hum_infectious_res_id.GetName() );
        channelDataMap.RemoveChannel( log_prev_id.GetName()           );
        channelDataMap.RemoveChannel( ymc_id.GetName()                );
        channelDataMap.RemoveChannel( yfc_id.GetName()                );

        if( m_IncludeCoitalActs )
        {
            for( int i = 0 ; i < active_rel_ids.size(); ++i )
            {
                normalizeChannel( acts_this_dt_rel_ids[ i ].GetName(),         active_rel_ids[ i ].GetName());
                normalizeChannel( acts_this_dt_condoms_rel_ids[ i ].GetName(), active_rel_ids[ i ].GetName());
            }
        }

        normalizeChannel( infs_outside_pfa_id.GetName(), Report::new_infections_id.GetName() );
    }
}
