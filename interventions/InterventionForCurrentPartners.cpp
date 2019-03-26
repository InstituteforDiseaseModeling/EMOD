/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "InterventionForCurrentPartners.h"

#include <typeinfo>

#include "IIndividualHumanContext.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "InterventionValidator.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "IIndividualHumanSTI.h"
#include "Common.h"
#include "RANDOM.h"

SETUP_LOGGING( "InterventionForCurrentPartners" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( InterventionForCurrentPartners )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( InterventionForCurrentPartners )

    IMPLEMENT_FACTORY_REGISTERED( InterventionForCurrentPartners )

    InterventionForCurrentPartners::InterventionForCurrentPartners()
        : BaseIntervention()
        , m_RelationshipTypes()
        , m_PrioritizePartnersBy( PartnerPrioritizationType::NO_PRIORITIZATION )
        , m_MinimumDurationYears( 0.0f )
        , m_MinimumDurationDays( 0.0f )
        , m_MaximumPartners( 1000.0f )
        , m_UseEventOrConfig( EventOrConfig::Event )
        , m_EventToBroadcast()
        , m_InterventionConfig()
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    InterventionForCurrentPartners::InterventionForCurrentPartners( const InterventionForCurrentPartners& rMaster )
        : BaseIntervention( rMaster )
        , m_RelationshipTypes(    rMaster.m_RelationshipTypes )
        , m_PrioritizePartnersBy( rMaster.m_PrioritizePartnersBy )
        , m_MinimumDurationYears( rMaster.m_MinimumDurationYears )
        , m_MinimumDurationDays(  rMaster.m_MinimumDurationDays )
        , m_MaximumPartners(      rMaster.m_MaximumPartners )
        , m_UseEventOrConfig(     rMaster.m_UseEventOrConfig )
        , m_EventToBroadcast(     rMaster.m_EventToBroadcast )
        , m_InterventionConfig(   rMaster.m_InterventionConfig )
    {
    }

    InterventionForCurrentPartners::~InterventionForCurrentPartners()
    {
    }

    bool InterventionForCurrentPartners::Configure( const Configuration * inputJson )
    {
        std::set<std::string> allowable_relationship_types = GetAllowableRelationshipTypes();
        std::vector<std::string> rel_type_strings;
        initConfigTypeMap( "Relationship_Types", &rel_type_strings, IFCP_Relationship_Types_DESC_TEXT, nullptr, allowable_relationship_types );

        initConfigTypeMap( "Minimum_Duration_Years", &m_MinimumDurationYears, IFCP_Minimum_Duration_Years_DESC_TEXT, 0.0, 200, 0.0 );
        initConfigTypeMap( "Maximum_Partners", &m_MaximumPartners, IFCP_Maximum_Partners_DESC_TEXT, 0.0, 100.0, 100.0 );

        initConfig( "Prioritize_Partners_By", m_PrioritizePartnersBy, inputJson, MetadataDescriptor::Enum( "Prioritize_Partners_By", IFCP_Prioritize_Partners_By_DESC_TEXT, MDD_ENUM_ARGS( PartnerPrioritizationType ) ) );

        initConfig( "Event_Or_Config", m_UseEventOrConfig, inputJson, MetadataDescriptor::Enum( "EventOrConfig", Event_Or_Config_DESC_TEXT, MDD_ENUM_ARGS( EventOrConfig ) ) );
        initConfigTypeMap( "Broadcast_Event", &m_EventToBroadcast, IFCP_Broadcast_Event_DESC_TEXT, "Event_Or_Config", "Event" );
        initConfigComplexType( "Intervention_Config", &m_InterventionConfig, IFCP_Intervention_Config_DESC_TEXT, "Event_Or_Config", "Config" );

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( (m_UseEventOrConfig == EventOrConfig::Event) && m_EventToBroadcast.IsUninitialized() )
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                        "If you set 'Event_Or_Config' = 'Event', then you must define 'Broadcast_Event'" );
            }
            if( m_UseEventOrConfig == EventOrConfig::Config )
            {
                if( m_InterventionConfig._json.Type() == ElementType::NULL_ELEMENT )
                {
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__,
                                                            "If you set 'Event_Or_Config' = 'Config', then you must define 'Intervention_Config'" );
                }
                else
                {
                    InterventionValidator::ValidateIntervention( GetTypeName(),
                                                                 InterventionTypeValidation::INDIVIDUAL,
                                                                 m_InterventionConfig._json,
                                                                 inputJson->GetDataLocation() );
                }
            }
            m_RelationshipTypes = ConvertStringsToRelationshipTypes( "Relationship_Types", rel_type_strings );
            m_MinimumDurationDays = m_MinimumDurationYears * DAYSPERYEAR;
        }
        return ret;
    }

    void InterventionForCurrentPartners::Update( float dt )
    {
        expired = true;

        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        IIndividualHumanSTI* p_human_sti = nullptr;
        if( s_OK != parent->QueryInterface( GET_IID( IIndividualHumanSTI ), (void**)&p_human_sti ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanSTI", "IIndividualHumanContext" );
        }

        RelationshipSet_t relationships = p_human_sti->GetRelationships();

        std::vector<IRelationship*> reduced_relationships = SelectRelationships( relationships );

        std::vector<IIndividualHumanEventContext*> partners = SelectPartners( p_human_sti, reduced_relationships );

        ReducePartners( partners );

        if( m_UseEventOrConfig == EventOrConfig::Event )
        {
            DistributeToPartnersEvent( partners );
        }
        else
        {
            DistributeToPartnersIntervention( partners );
        }
    }

    std::vector<IRelationship*> 
        InterventionForCurrentPartners::SelectRelationships( const RelationshipSet_t& rRelationships )
    {
        // --------------------------------------------------------------------------
        // --- Only include relationships whose duration is greater than the minimum
        // --------------------------------------------------------------------------
        std::vector<IRelationship*> reduced_relationships;
        for( auto p_rel : rRelationships )
        {
            if( (p_rel->GetState() == RelationshipState::NORMAL) &&
                (m_MinimumDurationDays <= p_rel->GetDuration()) )
            {
                reduced_relationships.push_back( p_rel );
            }
        }
        return reduced_relationships;
    }

    std::vector<IIndividualHumanEventContext*> 
        InterventionForCurrentPartners::SelectPartners( IIndividualHumanSTI* pHumanStiSelf,
                                                       std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners;

        switch( m_PrioritizePartnersBy )
        {
            case PartnerPrioritizationType::NO_PRIORITIZATION:
                partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::CHOSEN_AT_RANDOM:
                partners = SelectPartnersChosenAtRandom( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::LONGER_TIME_IN_RELATIONSHIP:
                partners = SelectPartnersLongerTime( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::SHORTER_TIME_IN_RELATIONSHIP:
                partners = SelectPartnersShorterTime( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::OLDER_AGE:
                partners = SelectPartnersOlderAge( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::YOUNGER_AGE:
                partners = SelectPartnersYoungerAge( pHumanStiSelf, reducedRelationships );
                break;
            case PartnerPrioritizationType::RELATIONSHIP_TYPE:
                partners = SelectPartnersRelationshipType( pHumanStiSelf, reducedRelationships );
                break;
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__,
                                                         "m_PrioritizePartnersBy",
                                                         m_PrioritizePartnersBy,
                                                         PartnerPrioritizationType::pairs::lookup_key( m_PrioritizePartnersBy ) );
        }
        return partners;
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersNoPrioritization( IIndividualHumanSTI* pHumanStiSelf,
                                                                        std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners;
        for( auto p_rel : reducedRelationships )
        {
            IIndividualHumanSTI* p_human_sti_partner = p_rel->GetPartner( pHumanStiSelf );
            IIndividualHumanEventContext* p_human_event = nullptr;
            if( p_human_sti_partner->QueryInterface( GET_IID( IIndividualHumanEventContext ), (void**)&p_human_event ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                               "p_human_sti_partner",
                                               "IIndividualHumanEventContext",
                                               "IIndividualHumanSTI" );
            }
            partners.push_back( p_human_event );
        }
        return partners;
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersChosenAtRandom( IIndividualHumanSTI* pHumanStiSelf,
                                                                      std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );

        RANDOMBASE* p_rng = parent->GetRng();
        auto myran = [p_rng] ( int i ) { return p_rng->uniformZeroToN32( i ); };
        std::random_shuffle( partners.begin(), partners.end(), myran );

        return partners;
    }

    bool LongerTime( IRelationship* pRelA, IRelationship* pRelB )
    {
        return pRelA->GetDuration() < pRelB->GetDuration();
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersLongerTime( IIndividualHumanSTI* pHumanStiSelf,
                                                                  std::vector<IRelationship*>& reducedRelationships )
    {
        std::sort( reducedRelationships.begin(), reducedRelationships.end(), LongerTime );
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
        return partners;
    }

    bool ShorterTime( IRelationship* pRelA, IRelationship* pRelB )
    {
        return pRelA->GetDuration() > pRelB->GetDuration();
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersShorterTime( IIndividualHumanSTI* pHumanStiSelf,
                                                                   std::vector<IRelationship*>& reducedRelationships )
    {
        std::sort( reducedRelationships.begin(), reducedRelationships.end(), ShorterTime );
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
        return partners;
    }

    bool OlderAge( IIndividualHumanEventContext* pHumanA, IIndividualHumanEventContext* pHumanB )
    {
        return pHumanA->GetAge() < pHumanB->GetAge();
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersOlderAge( IIndividualHumanSTI* pHumanStiSelf,
                                                                std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
        std::sort( partners.begin(), partners.end(), OlderAge );
        return partners;
    }

    bool YoungerAge( IIndividualHumanEventContext* pHumanA, IIndividualHumanEventContext* pHumanB )
    {
        return pHumanA->GetAge() > pHumanB->GetAge();
    }

    std::vector<IIndividualHumanEventContext*>
        InterventionForCurrentPartners::SelectPartnersYoungerAge( IIndividualHumanSTI* pHumanStiSelf,
                                                                  std::vector<IRelationship*>& reducedRelationships )
    {
        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, reducedRelationships );
        std::sort( partners.begin(), partners.end(), YoungerAge );
        return partners;
    }

    std::vector<IIndividualHumanEventContext*> 
        InterventionForCurrentPartners::SelectPartnersRelationshipType( IIndividualHumanSTI* pHumanStiSelf,
                                                                        std::vector<IRelationship*>& reducedRelationships )
    {
        RANDOMBASE* p_rng = parent->GetRng();
        auto myran = [p_rng] ( int i ) { return p_rng->uniformZeroToN32( i ); };

        std::vector<IRelationship*> sorted_rels;
        for( auto rel_type : m_RelationshipTypes )
        {
            // ------------------------------------------
            // --- find the relationships of a given type
            // ------------------------------------------
            std::vector<IRelationship*> sorted_rels_by_type;
            for( int i = 0 ; i < reducedRelationships.size() ; )
            {
                if( rel_type == reducedRelationships[ i ]->GetType() )
                {
                    sorted_rels_by_type.push_back( reducedRelationships[ i ] );
                    reducedRelationships[ i ] = reducedRelationships.back();
                    reducedRelationships.pop_back();
                }
                else
                {
                    ++i;
                }
            }

            // ----------------------------------------------------------------------
            // --- multiple relationships of the same type will be randomly selected
            // ----------------------------------------------------------------------
            std::random_shuffle( sorted_rels_by_type.begin(), sorted_rels_by_type.end(), myran );

            // ------------------------------------------------------------------------------
            // --- Append the relationships for the current type to the end of the total list
            // ------------------------------------------------------------------------------
            sorted_rels.insert( sorted_rels.end(), sorted_rels_by_type.begin(), sorted_rels_by_type.end() );
        }

        std::vector<IIndividualHumanEventContext*> partners = SelectPartnersNoPrioritization( pHumanStiSelf, sorted_rels );
        return partners;
    }

    void InterventionForCurrentPartners::ReducePartners( std::vector<IIndividualHumanEventContext*>& partners )
    {
        // -----------------------------------------------------------------------
        // --- randomly round to the nearest integer such that if m_MaximumPartners is 2.3, 
        // --- it returns two 70% of the time and three 30%.
        // -----------------------------------------------------------------------
        int max = parent->GetRng()->randomRound( m_MaximumPartners );
        while( partners.size() > max )
        {
            partners.pop_back();
        }
    }

    void InterventionForCurrentPartners::DistributeToPartnersEvent( const std::vector<IIndividualHumanEventContext*>& partners )
    {
        IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
        for( auto p_human_event : partners )
        {
            broadcaster->TriggerObservers( p_human_event, m_EventToBroadcast );
        }
    }

    void InterventionForCurrentPartners::DistributeToPartnersIntervention( const std::vector<IIndividualHumanEventContext*>& partners )
    {
        ICampaignCostObserver* pICCO;
        if( parent->GetEventContext()->GetNodeEventContext()->QueryInterface( GET_IID( ICampaignCostObserver ), (void**)&pICCO ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__,
                                           "parent->GetEventContext()->GetNodeEventContext()",
                                           "ICampaignCostObserver",
                                           "INodeEventContext" );
        }

        auto config = Configuration::CopyFromElement( (m_InterventionConfig._json), "campaign" );
        IDistributableIntervention *di = InterventionFactory::getInstance()->CreateIntervention( config );
        delete config;
        config = nullptr;

        for( auto p_human_event : partners )
        {
            IDistributableIntervention *clone_di = di->Clone();
            clone_di->AddRef();
            clone_di->Distribute( p_human_event->GetInterventionsContext(), pICCO );
            clone_di->Release();
        }
    }

    REGISTER_SERIALIZABLE( InterventionForCurrentPartners );

    void InterventionForCurrentPartners::serialize( IArchive& ar, InterventionForCurrentPartners* obj )
    {
        BaseIntervention::serialize( ar, obj );
        InterventionForCurrentPartners& ifcp = *obj;

        std::vector<uint32_t> rel_types;
        for( auto rel : ifcp.m_RelationshipTypes )
        {
            rel_types.push_back( uint32_t(rel) );
        }

        ar.labelElement( "m_RelationshipTypes"    ) & rel_types;
        ar.labelElement( "m_PrioritizePartnersBy" ) & (uint32_t&)ifcp.m_PrioritizePartnersBy;
        ar.labelElement( "m_MinimumDurationYears" ) & ifcp.m_MinimumDurationYears;
        ar.labelElement( "m_MinimumDurationDays"  ) & ifcp.m_MinimumDurationDays;
        ar.labelElement( "m_MaximumPartners"      ) & ifcp.m_MaximumPartners;
        ar.labelElement( "m_UseEventOrConfig"     ) & (uint32_t&)ifcp.m_UseEventOrConfig;
        ar.labelElement( "m_EventToBroadcast"     ) & ifcp.m_EventToBroadcast;
        //ar.labelElement( "m_InterventionConfig"   ) & ifcp.m_InterventionConfig;

        // Haven't tested this so assert if someone tries it but I don't think this will
        // ever be serialized in practice.
        release_assert( ifcp.m_InterventionConfig._json.Type() == ElementType::NULL_ELEMENT );

        ifcp.m_RelationshipTypes.clear();
        for( auto irel : rel_types )
        {
            ifcp.m_RelationshipTypes.push_back( RelationshipType::Enum(irel) );
        }
    }
}
