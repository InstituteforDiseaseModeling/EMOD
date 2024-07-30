
#include "stdafx.h"

#include <string>
#include "StiRelationshipStartReporter.h"
#include "Log.h"
#include "Exceptions.h"
#include "INodeSTI.h"
#include "Properties.h"
#include "NodeEventContext.h"
#include "IIndividualHumanSTI.h"
#include "INodeContext.h"
#include "ReportUtilitiesSTI.h"

SETUP_LOGGING( "StiRelationshipStartReporter" )

using namespace std;

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL( StiRelationshipStartReporter, StiRelationshipStartReporter )

    IReport* StiRelationshipStartReporter::Create(ISimulation* simulation)
    {
        return new StiRelationshipStartReporter(simulation);
    }

    StiRelationshipStartReporter::StiRelationshipStartReporter(ISimulation* sim)
        : BaseTextReport("RelationshipStart.csv")
        , simulation(sim)
        , report_data()
        , m_IPKeyNames()
        , m_IPKeys()
        , m_IncludeOther( true )
        , m_ReportFilter( "Report_Relationship_Start", "Report_Relationship_Start" )
    {
        if( simulation != nullptr )
        {
            simulation->RegisterNewNodeObserver( this, [ & ]( INodeContext* node ) { this->onNewNode( node ); } );
        }
    }

    StiRelationshipStartReporter::~StiRelationshipStartReporter()
    {
        simulation->UnregisterNewNodeObserver(this);
    }

    bool StiRelationshipStartReporter::Configure( const Configuration* inputJson )
    {
        m_ReportFilter.ConfigureParameters( *this, inputJson );

        initConfigTypeMap( "Report_Relationship_Start_Individual_Properties", &m_IPKeyNames, RelStart_Individual_Properties_DESC_TEXT, nullptr, JsonConfigurable::empty_set, "Report_Relationship_Start" );
        initConfigTypeMap( "Report_Relationship_Start_Include_Other_Relationship_Statistics", &m_IncludeOther, RelStart_Include_Other_Relationship_Statistics_DESC_TEXT, true,"Report_Relationship_Start" );
        
        bool ret = JsonConfigurable::Configure( inputJson );

        if( ret && !JsonConfigurable::_dryrun )
        {
            m_ReportFilter.CheckParameters( inputJson );
        }
        return ret;
    }

    void StiRelationshipStartReporter::Initialize( unsigned int nrmSize )
    {
        for( auto name : m_IPKeyNames )
        {
            IPKey key( name );
            m_IPKeys.push_back( key );
        }
        m_ReportFilter.Initialize();
        BaseTextReport::Initialize( nrmSize );
    }

    void StiRelationshipStartReporter::CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& demographicNodeIds )
    {
        m_ReportFilter.CheckForValidNodeIDs( "RelationshipStart.csv", demographicNodeIds );
    }

    void StiRelationshipStartReporter::onNewNode(INodeContext* node)
    {
        INodeSTI* sti = nullptr;

        if (node->QueryInterface(GET_IID(INodeSTI), (void**)&sti) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "node", "INodeSTI*", "INodeContext*" );
        }

        auto manager = sti->GetRelationshipManager();
        manager->RegisterNewRelationshipObserver([&](IRelationship* relationship){ this->onNewRelationship(relationship); });
    }

    void StiRelationshipStartReporter::onNewRelationship(IRelationship* relationship)
    {
        if( !m_ReportFilter.IsValidRelationship( relationship ) )
        {
            return;
        }

        LOG_DEBUG_F("%s: rel id = %d, male id = %d, female id = %d\n", __FUNCTION__,
                    relationship->GetSuid().data,
                    relationship->MalePartner()->GetSuid().data,
                    relationship->FemalePartner()->GetSuid().data );
        // TODO - set the relationship suid in the relationship code (or relationship manager)
        auto male_partner = relationship->MalePartner();
        auto female_partner    = relationship->FemalePartner();

        if (male_partner && female_partner)
        {
            RelationshipStartInfo info;
            info.id                 = relationship->GetSuid().data;
            info.start_time         = relationship->GetStartTime();
            info.scheduled_end_time = relationship->GetScheduledEndTime();
            info.relationship_type  = (unsigned int)relationship->GetType();
            info.is_outside_pfa     = relationship->IsOutsidePFA();

            IIndividualHumanEventContext* individual = nullptr;

            if (male_partner->QueryInterface(GET_IID(IIndividualHumanEventContext), (void**)&individual) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "male_partner", "IIndividualHumanContext", "IIndividualHumanSTI*" );
            }

            // --------------------------------------------------------
            // --- Assuming that the individuals in a relationship
            // --- must be in the same node.
            // --------------------------------------------------------
            info.original_node_id = relationship->GetOriginalNodeId();
            info.current_node_id  = individual->GetNodeEventContext()->GetNodeContext()->GetExternalID();

            ExtractPartnerData( individual, male_partner, info.participant_a );

            if (female_partner->QueryInterface(GET_IID(IIndividualHumanEventContext), (void**)&individual) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "female_partner", "IIndividualHumanContext", "IIndividualHumanSTI*" );
            }

            ExtractPartnerData( individual, female_partner, info.participant_b );

            CollectOtherData( info.id, male_partner, female_partner );

            report_data.push_back(info);
        }
        else
        {
            LOG_WARN_F( "%s: one or more partners of new relationship %d has already migrated\n", __FUNCTION__, relationship->GetSuid().data );
        }
    }

    void StiRelationshipStartReporter::ExtractPartnerData( IIndividualHumanEventContext* pHuman,
                                                           IIndividualHumanSTI* pHumanSTI,
                                                           ParticipantInfo& rParticipant )
    {
        rParticipant.id                        = pHumanSTI->GetSuid().data;
        rParticipant.is_infected               = pHumanSTI->IsInfected();
        rParticipant.gender                    = pHuman->GetGender();
        rParticipant.age                       = pHuman->GetAge() / 365;
        rParticipant.previous_coita_acts_count = pHumanSTI->GetTotalCoitalActs();
        rParticipant.active_relationship_count = pHumanSTI->GetRelationships().size();

        for( auto& r_key : m_IPKeys )
        {
            rParticipant.props.push_back( pHuman->GetProperties()->Get( r_key ).GetValueAsString() );
        }

        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            rParticipant.relationship_count[ i ] = 0;
        }

        for( auto relationship : pHumanSTI->GetRelationships() )
        {
            rParticipant.relationship_count[ int( relationship->GetType() ) ]++;
        }

        rParticipant.cumulative_lifetime_relationships = pHumanSTI->GetLifetimeRelationshipCount();
        rParticipant.relationships_in_last_six_months  = pHumanSTI->GetLast6MonthRels();
        rParticipant.extrarelational_flags             = pHumanSTI->GetExtrarelationalFlags();
        rParticipant.is_circumcised                    = pHumanSTI->IsCircumcised();
        rParticipant.has_sti                           = pHumanSTI->HasSTICoInfection();
        rParticipant.is_superspreader                  = pHumanSTI->IsBehavioralSuperSpreader();
    }

    std::string StiRelationshipStartReporter::GetHeader() const
    {
        std::stringstream header ;
        header
            << "Rel_ID,"
            << "Rel_start_time,"
            << "Rel_scheduled_end_time,"
            << ReportUtilitiesSTI::GetRelationshipTypeColumnHeader() << ","
            << "Is_rel_outside_PFA,"
            << "Original_node_ID,"
            << "Current_node_ID";

        AddPartnerColumnHeaders( "A", header );
        AddPartnerColumnHeaders( "B", header );

        return header.str();
    }

    void StiRelationshipStartReporter::AddPartnerColumnHeaders( const char* pPartnerLabel, std::stringstream& rHeader ) const
    {
        rHeader
            << "," << pPartnerLabel << "_ID"
            << "," << pPartnerLabel << "_is_infected"
            << "," << pPartnerLabel << "_gender"
            << "," << pPartnerLabel << "_age";

        for( auto& r_key : m_IPKeys )
        {
            rHeader << "," << pPartnerLabel << "_IP='" << r_key.ToString() << "'";
        }

        if( m_IncludeOther )
        {
            rHeader
                << "," << pPartnerLabel << "_previous_num_coital_acts"
                << "," << pPartnerLabel << "_total_num_active_rels";

            for( int i = 0; i < RelationshipType::COUNT; ++i )
            {
                rHeader << "," << pPartnerLabel << "_num_active_" << RelationshipType::pairs::get_keys()[ i ] << "_rels";
            }

            rHeader
                << "," << pPartnerLabel << "_num_lifetime_rels"
                << "," << pPartnerLabel << "_num_rels_last_6_mo"
                << "," << pPartnerLabel << "_extra_relational_bitmask";
        }
        rHeader
            << "," << pPartnerLabel << "_is_circumcised"
            << "," << pPartnerLabel << "_has_STI_coinfection"
            << "," << pPartnerLabel << "_is_superspreader";
    }

    void StiRelationshipStartReporter::BeginTimestep()
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        ClearData();
    }

    void StiRelationshipStartReporter::EndTimestep( float currentTime, float dt )
    {
        LOG_DEBUG_F("%s\n", __FUNCTION__);
        // TODO - per time step data reduction (if multi-core)
        for (auto& entry : report_data)
        {
            GetOutputStream()
                << entry.id
                << "," << entry.start_time
                << "," << entry.scheduled_end_time
                << "," << entry.relationship_type
                << "," << (entry.is_outside_pfa ? 'T' : 'F')
                << "," << entry.original_node_id
                << "," << entry.current_node_id;

            WriteParticipant( entry.participant_a );
            WriteParticipant( entry.participant_b );

            GetOutputStream() << GetOtherData( entry.id )
                              << endl;
        }

        BaseTextReport::EndTimestep( currentTime, dt );
    }

    void StiRelationshipStartReporter::WriteParticipant( ParticipantInfo& rParticipant )
    {
        GetOutputStream()
            << "," << rParticipant.id
            << "," << rParticipant.is_infected
            << "," << rParticipant.gender
            << "," << rParticipant.age;

        for( auto value : rParticipant.props )
        {
            GetOutputStream() << "," << value;
        }

        if( m_IncludeOther )
        {
            GetOutputStream() << "," << rParticipant.previous_coita_acts_count;
            GetOutputStream() << "," << rParticipant.active_relationship_count;

            for( int i = 0; i < RelationshipType::COUNT; ++i )
            {
                GetOutputStream() << "," << rParticipant.relationship_count[ i ];
            }

            GetOutputStream()
                << "," << rParticipant.cumulative_lifetime_relationships
                << "," << rParticipant.relationships_in_last_six_months
                << "," << rParticipant.extrarelational_flags;
        }
        GetOutputStream()
            << "," << rParticipant.is_circumcised
            << "," << rParticipant.has_sti
            << "," << rParticipant.is_superspreader;
    }

    void StiRelationshipStartReporter::ClearData()
    {
        report_data.clear();
    }
}
