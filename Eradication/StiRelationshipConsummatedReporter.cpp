
#include "stdafx.h"

#include <string>
#include "StiRelationshipConsummatedReporter.h"
#include "Log.h"
#include "Exceptions.h"
#include "INodeSTI.h"
#include "IIndividualHuman.h"
#include "INodeContext.h"
#include "IRelationship.h"
#include "RelationshipReporting.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanSTI.h"
#include "IIndividualHumanHIV.h"
#include "InfectionHIV.h"
#include "Interventions.h"
#include "IHIVInterventionsContainer.h"
#include "INodeContext.h"
#include "ReportUtilitiesSTI.h"
#include "IndividualEventContext.h"
#include "SimulationEventContext.h"

SETUP_LOGGING( "RelationshipConsummatedReporter" )

using namespace std;

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL( StiRelationshipConsummatedReporter, StiRelationshipConsummatedReporter )

    IReport* StiRelationshipConsummatedReporter::Create(ISimulation* simulation)
    {
        return new StiRelationshipConsummatedReporter(simulation);
    }

    StiRelationshipConsummatedReporter::StiRelationshipConsummatedReporter(ISimulation* sim)
        : BaseTextReportEvents("RelationshipConsummated.csv")
        , m_pSimulation(sim)
        , m_IsCollectingData(false)
        , m_ReportData()
        , m_InterventionNames()
        , m_IPKeyNames()
        , m_IPKeys()
        , m_PartnersWithIPKeyValueNames()
        , m_PartnersWithIPKeyValue()
        , m_ReportFilter( "Report_Coital_Acts", "Report_Coital_Acts" )
        , m_RelationshipType( ReportRelationshipType::NA )
    {
        if( m_pSimulation != nullptr )
        {
            sim->RegisterNewNodeObserver( this, [ & ]( INodeContext* node ) { this->onNewNode( node ); } );
        }

        eventTriggerList.push_back( EventTrigger::STIExposed );
    }

    StiRelationshipConsummatedReporter::~StiRelationshipConsummatedReporter()
    {
        m_pSimulation->UnregisterNewNodeObserver(this);
    }

    bool StiRelationshipConsummatedReporter::Configure( const Configuration* inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        initConfig( "Report_Coital_Acts_Relationship_Type", m_RelationshipType, inputJson,
                    MetadataDescriptor::Enum("Report_Coital_Acts_Relationship_Type",
                                              RCA_Relationship_Type_DESC_TEXT,
                                              MDD_ENUM_ARGS(ReportRelationshipType)) );

        std::vector<std::string> tmp_intervention_names;
        initConfigTypeMap( "Report_Coital_Acts_Has_Intervention_With_Name", &tmp_intervention_names,        RCA_Has_Intervention_With_Name_DESC_TEXT, "Report_Coital_Acts" );
        initConfigTypeMap( "Report_Coital_Acts_Individual_Properties",      &m_IPKeyNames,                  RCA_Individual_Properties_DESC_TEXT,      "Report_Coital_Acts" );
        initConfigTypeMap( "Report_Coital_Acts_Partners_With_IP_Key_Value", &m_PartnersWithIPKeyValueNames, RCA_Partners_With_IP_Key_Value_DESC_TEXT, "Report_Coital_Acts" );

        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            for( auto tmp_name : tmp_intervention_names )
            {
                m_InterventionNames.push_back( InterventionName( tmp_name ) );
            }
            m_ReportFilter.CheckParameters( inputJson );
        }
        return ret;
    }

    void StiRelationshipConsummatedReporter::Initialize( unsigned int nrmSize )
    {
        for( auto name : m_IPKeyNames )
        {
            IPKey key( name );
            m_IPKeys.push_back( key );
        }
        for( auto name : m_PartnersWithIPKeyValueNames )
        {
            IPKeyValue kv( name );
            m_PartnersWithIPKeyValue.push_back( kv );
        }
        m_ReportFilter.Initialize();
        BaseTextReport::Initialize( nrmSize );
    }

    void StiRelationshipConsummatedReporter::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds )
    {
        m_ReportFilter.CheckForValidNodeIDs( GetReportName(), demographicNodeIds );
    }

    void StiRelationshipConsummatedReporter::UpdateEventRegistration( float currentTime,
                                                                      float dt,
                                                                      std::vector<INodeEventContext*>& rNodeEventContextList,
                                                                      ISimulationEventContext* pSimEventContext )
    {
        bool is_valid_time = m_ReportFilter.IsValidTime( pSimEventContext->GetSimulationTime() );
        if( !m_IsCollectingData && is_valid_time )
        {
            m_IsCollectingData = true;
            BaseTextReportEvents::UpdateEventRegistration( currentTime, dt, rNodeEventContextList, pSimEventContext );
        }
        else if( m_IsCollectingData && !is_valid_time )
        {
            UnregisterAllBroadcasters();
            m_IsCollectingData = false;
        }
    }

    void StiRelationshipConsummatedReporter::onNewNode(Kernel::INodeContext* node)
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! We can't check the node ID here because we haven't validated the node ids in the report yet.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        INodeSTI* sti = nullptr;

        if (node->QueryInterface(GET_IID(INodeSTI), (void**)&sti) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeSTI*", "INodeContext*" );
        }

        auto observer_func = [ & ]( IRelationship* relationship, const CoitalAct& rCoitalAct )
        {
            this->onCoitalAct( relationship, rCoitalAct );
        };
        sti->GetRelationshipManager()->RegisterRelationshipConsummationObserver( observer_func );
    }

    void StiRelationshipConsummatedReporter::onCoitalAct( IRelationship* relationship, const CoitalAct& rCoitalAct )
    {
        if( !m_IsCollectingData )
        {
            return;
        }

        if( (m_RelationshipType != ReportRelationshipType::NA) && (m_RelationshipType != relationship->GetType()) )
        {
            return;
        }

        IIndividualHuman* p_male   = relationship->MalePartner()->GetIndividualHuman();
        IIndividualHuman* p_female = relationship->FemalePartner()->GetIndividualHuman();

        float male_age_years   = p_male->GetAge()   / DAYSPERYEAR;
        float female_age_years = p_female->GetAge() / DAYSPERYEAR;

        bool include_data = m_ReportFilter.IsValidRelationship( relationship );
        if( include_data )
        {
            ConsummatedReportData data = GatherDataFromRelationship( relationship, rCoitalAct );
            m_ReportData.insert( std::make_pair( rCoitalAct.GetSuid().data, data ) );
        }
    }

    std::string StiRelationshipConsummatedReporter::GetHeader() const
    {
        std::ostringstream header;
        header
            << "Time"
            << ",Year"
            << ",Node_ID"
            << ",Coital_Act_ID"
            << ",Rel_ID"
            << "," << ReportUtilitiesSTI::GetRelationshipTypeColumnHeader()
            << ",Is_rel_outside_PFA"
            << ",A_ID"
            << ",B_ID"
            << ",A_Gender"
            << ",B_Gender"
            << ",A_Age"
            << ",B_Age";
        for( auto& r_key : m_IPKeys )
        {
            header << ",A_IP='" << r_key.ToString() << "'";
            header << ",B_IP='" << r_key.ToString() << "'";
        }
        for( auto& r_kv : m_PartnersWithIPKeyValue )
        {
            header << ",A_PartnersWith_IP='" << r_kv.ToString() << "'";
            header << ",B_PartnersWith_IP='" << r_kv.ToString() << "'";
        }
        for( auto& r_name : m_InterventionNames )
        {
            header << ",A_HasIntervention='" << r_name.ToString() << "'";
            header << ",B_HasIntervention='" << r_name.ToString() << "'";
        }
        header
            << ",A_Is_Infected"
            << ",B_Is_Infected"
            << ",Did_Use_Condom"
            << ",Risk_Multiplier"
            << ",Transmission_Multiplier"
            << ",Acquisition_Multiplier"
            << ",Infection_Was_Transmitted"
            << ",A_Num_Current_Rels"
            << ",B_Num_Current_Rels"
            << ",A_Is_Circumcised"
            << ",B_Is_Circumcised"
            << ",A_Has_CoInfection"
            << ",B_Has_CoInfection"
            // The following are HIV-Specific
            << ",A_HIV_Infection_Stage"
            << ",B_HIV_Infection_Stage"
            << ",A_Is_On_ART"
            << ",B_Is_On_ART";
        std::string retLine = header.str();
        return retLine;
    }

    void StiRelationshipConsummatedReporter::ExtractData( ConsummatedIndividulatData& rIndividualData,
                                                          IIndividualHumanSTI* pHumanSTI )
    {
        IIndividualHuman *p_human = pHumanSTI->GetIndividualHuman();

        rIndividualData.human_id  = p_human->GetSuid().data;
        rIndividualData.gender    = p_human->GetGender();
        rIndividualData.age_years = p_human->GetAge() / double( DAYSPERYEAR );

        rIndividualData.is_infected               = pHumanSTI->IsInfected();
        rIndividualData.num_current_relationships = pHumanSTI->GetRelationships().size();
        rIndividualData.is_circumcised            = pHumanSTI->IsCircumcised();
        rIndividualData.has_sti_coinfection       = pHumanSTI->HasSTICoInfection();

        for( auto& r_key : m_IPKeys )
        {
            rIndividualData.ip_values.push_back( p_human->GetProperties()->Get( r_key ).GetValueAsString() );
        }

        std::vector<IRelationship*>& r_relationships = pHumanSTI->GetRelationships();
        for( auto& r_kv : m_PartnersWithIPKeyValue )
        {
            uint32_t num_partners = 0;
            for( IRelationship* p_rel : r_relationships )
            {
                // only NORMAL because we don't have access to migrated partner
                if( p_rel->GetState() == RelationshipState::NORMAL )
                {
                    IIndividualHumanSTI* p_partner = p_rel->GetPartner( pHumanSTI );
                    if( p_partner->GetIndividualHuman()->GetProperties()->Contains( r_kv ) )
                    {
                        num_partners += 1;
                    }
                }
            }
            rIndividualData.num_partners_with_IP.push_back( num_partners );
        }

        for( auto& r_name : m_InterventionNames )
        {
            rIndividualData.has_intervention.push_back( p_human->GetInterventionsContext()->ContainsExistingByName( r_name ) );
        }

        IIndividualHumanHIV *p_human_hiv = nullptr;
        if( pHumanSTI->IsInfected() && (pHumanSTI->QueryInterface( GET_IID( IIndividualHumanHIV ), (void**)&p_human_hiv ) == s_OK) )
        {
            rIndividualData.HIV_infection_stage = p_human_hiv->GetHIVInfection()->GetStage();
            rIndividualData.is_on_art           = p_human_hiv->GetHIVInterventionsContainer()->OnArtQuery();
        }
    }

    ConsummatedReportData StiRelationshipConsummatedReporter::GatherDataFromRelationship( const IRelationship* pRel, const CoitalAct& rCoitalAct )
    {
        release_assert( pRel );
        IIndividualHumanSTI *sti_A = pRel->MalePartner();
        IIndividualHumanSTI *sti_B = pRel->FemalePartner();

        IIndividualHuman *ih_A = sti_A->GetIndividualHuman();

        // --------------------------------------------------------
        // --- Assuming that the individuals in a relationship
        // --- must be in the same node.
        // --------------------------------------------------------
        ConsummatedReportData data;
        data.time           = m_pSimulation->GetSimulationTime().time;
        data.year           = m_pSimulation->GetSimulationTime().Year();
        data.node_id        = ih_A->GetParent()->GetExternalID();
        data.coital_act_id  = rCoitalAct.GetSuid().data;
        data.rel_id         = pRel->GetSuid().data;
        data.rel_type       = pRel->GetType();
        data.is_outside_pfa = pRel->IsOutsidePFA();
        data.using_condom   = rCoitalAct.IsUsingCondoum();

        ExtractData( data.individual_data[ 0 ], sti_A );
        ExtractData( data.individual_data[ 1 ], sti_B );

        return data;
    }

    bool StiRelationshipConsummatedReporter::notifyOnEvent( IIndividualHumanEventContext *context, const EventTrigger& trigger )
    {
        if( trigger == EventTrigger::STIExposed )
        {
            IIndividualHumanSTI *p_human_sti = nullptr;
            if( context->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&p_human_sti ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IIndividualHumanEventContext", "IIndividualHumanSTI" );
            }
            const CoitalAct& r_current_act = p_human_sti->GetCurrentCoitalAct();
            release_assert( r_current_act.GetUnInfectedPartnerID().data == p_human_sti->GetSuid().data );

            // -------------------------------------------------------------------------------
            // --- The coital act might not be in the report data if the act was filtered out
            // -------------------------------------------------------------------------------
            auto it = m_ReportData.find( r_current_act.GetSuid().data );
            if( it != m_ReportData.end() )
            {
                release_assert( it->second.coital_act_id == r_current_act.GetSuid().data );

                it->second.risk_mult = r_current_act.GetRiskMultiplier();
                it->second.prob_tran = r_current_act.GetTransmissionProbability();
                it->second.prob_acq  = r_current_act.GetAcquisitionProbability();

                it->second.infection_was_transmitted = r_current_act.WasTransmitted();

                if( it->second.individual_data[ 0 ].human_id == p_human_sti->GetSuid().data )
                {
                    it->second.individual_data[ 0 ].is_infected = p_human_sti->IsInfected();
                }
                else
                {
                    it->second.individual_data[ 1 ].is_infected = p_human_sti->IsInfected();
                }
            }
        }
        return true;
    }

    std::string StiRelationshipConsummatedReporter::CreateReportLine( const ConsummatedReportData& rData )
    {
        const char* gender_A = Gender::pairs::lookup_key( rData.individual_data[ 0 ].gender );
        const char* gender_B = Gender::pairs::lookup_key( rData.individual_data[ 1 ].gender );

        std::stringstream line;
        line        << rData.time
             << "," << rData.year
             << "," << rData.node_id
             << "," << rData.coital_act_id
             << "," << rData.rel_id
             << "," << rData.rel_type
             << "," << (rData.is_outside_pfa ? 'T' : 'F')
             << "," << rData.individual_data[ 0 ].human_id
             << "," << rData.individual_data[ 1 ].human_id
             << "," << gender_A
             << "," << gender_B
             << "," << rData.individual_data[ 0 ].age_years
             << "," << rData.individual_data[ 1 ].age_years;

        for( int i = 0; i < m_IPKeys.size(); ++i )
        {
            line << "," << rData.individual_data[ 0 ].ip_values[ i ];
            line << "," << rData.individual_data[ 1 ].ip_values[ i ];
        }
        for( int i = 0; i < m_PartnersWithIPKeyValue.size(); ++i )
        {
            line << "," << rData.individual_data[ 0 ].num_partners_with_IP[ i ];
            line << "," << rData.individual_data[ 1 ].num_partners_with_IP[ i ];
        }
        for( int i = 0; i < m_InterventionNames.size(); ++i )
        {
            line << "," << rData.individual_data[ 0 ].has_intervention[ i ];
            line << "," << rData.individual_data[ 1 ].has_intervention[ i ];
        }

        line << "," << rData.individual_data[ 0 ].is_infected
             << "," << rData.individual_data[ 1 ].is_infected
             << "," << rData.using_condom
             << "," << rData.risk_mult
             << "," << rData.prob_tran
             << "," << rData.prob_acq
             << "," << rData.infection_was_transmitted
             << "," << rData.individual_data[ 0 ].num_current_relationships
             << "," << rData.individual_data[ 1 ].num_current_relationships
             << "," << rData.individual_data[ 0 ].is_circumcised
             << "," << rData.individual_data[ 1 ].is_circumcised
             << "," << rData.individual_data[ 0 ].has_sti_coinfection
             << "," << rData.individual_data[ 1 ].has_sti_coinfection
             << "," << rData.individual_data[ 0 ].HIV_infection_stage
             << "," << rData.individual_data[ 1 ].HIV_infection_stage
             << "," << rData.individual_data[ 0 ].is_on_art
             << "," << rData.individual_data[ 1 ].is_on_art
             << endl;

        return line.str();
    }

    void StiRelationshipConsummatedReporter::BeginTimestep()
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        m_ReportData.clear();
    }

    void StiRelationshipConsummatedReporter::EndTimestep( float currentTime, float dt )
    {
        if( m_IsCollectingData )
        {
            LOG_DEBUG_F( "%s\n", __FUNCTION__ );
            // TODO - per time step data reduction (if multi-core)
            for( auto& r_data : m_ReportData )
            {
                GetOutputStream() << CreateReportLine( r_data.second );
            }

            BaseTextReport::EndTimestep( currentTime, dt );
        }
    }
}
