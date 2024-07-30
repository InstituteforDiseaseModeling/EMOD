
#pragma once

#include "RANDOM.h"
#include "IndividualHumanContextFake.h"
#include "IndividualHumanInterventionsContextFake.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "RandomFake.h"
#include "Relationship.h"
#include "RelationshipParameters.h"

using namespace Kernel;

class FakeRelationship : public Relationship
{
public:
    FakeRelationship::FakeRelationship( RANDOMBASE* pRNG,
                                        const suids::suid& rRelId,
                                        IRelationshipParameters* pRelParams,
                                        IIndividualHumanSTI * husbandIn,
                                        IIndividualHumanSTI * wifeIn )
        : Relationship( pRNG, rRelId, pRelParams, husbandIn, wifeIn, false, nullptr )
    {
        release_assert( husbandIn->GetSuid().data != wifeIn->GetSuid().data );
    }

    virtual ~FakeRelationship()
    {
    }

    Relationship* Clone()
    {
        return new FakeRelationship( *this );
    }

protected:
    virtual ProbabilityNumber getProbabilityUsingCondomThisAct() const override
    {
        return 1.0f;
    }
};

struct PfaFixture
{
public:
    PfaFixture()
        : m_NEC()
        , m_NC( suids::suid(), &m_NEC )
        , m_relationship_list()
        , m_hic_list()
        , m_human_list()
        , m_RelParamsTransitory( RelationshipType::TRANSITORY )
        , m_RelParamsInformal( RelationshipType::INFORMAL )
        , m_RelParamsMarital( RelationshipType::MARITAL )
        , m_RelParamsCommercial( RelationshipType::COMMERCIAL )
        , m_NextSuidHuman()
        , m_NextSuidRelationship()
        , m_InfectedRng()
        , m_UseDefaults(false)
    {
        m_UseDefaults = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = true;

        m_NextSuidHuman.data = 1;
        m_NextSuidRelationship.data = 1;
        Environment::Finalize();
        Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );

        EventTriggerFactory::DeleteInstance();
        json::Object fakeConfigJson;
        Configuration * fakeConfigValid = Environment::CopyFromElement( fakeConfigJson );
        EventTriggerFactory::GetInstance()->Configure( fakeConfigValid );

        m_NEC.Initialize();
        m_NEC.SetContextTo( &m_NC );
    }

    ~PfaFixture()
    {
        JsonConfigurable::_useDefaults = m_UseDefaults;
        ClearData();
        Environment::Finalize();
    }

    void Register( IIndividualEventObserver* pIEO, EventTrigger trigger )
    {
        m_NEC.RegisterObserver( pIEO, trigger );
    }

    void ClearData()
    {
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Leak the memory to speed up the test
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if( m_human_list.size() < 100 )
        {
            for( auto rel : m_relationship_list )
            {
                delete rel ;
            }
            m_relationship_list.clear();

            for( auto hic : m_hic_list )
            {
                delete hic ;
            }
            m_hic_list.clear();

            for( auto human : m_human_list )
            {
                delete human ;
            }
            m_human_list.clear();
        }
    }

    FakeRelationship* AddRelationship( RANDOMBASE* pRNG, 
                                       IIndividualHumanSTI* male,
                                       IIndividualHumanSTI* female,
                                       RelationshipType::Enum relType = RelationshipType::TRANSITORY,
                                       float duration = 0.0 )
    {
        RelationshipParameters* p_rel_parms = &m_RelParamsTransitory;
        switch( relType )
        {
            case RelationshipType::TRANSITORY:
                p_rel_parms = &m_RelParamsTransitory;
                break;
            case RelationshipType::INFORMAL:
                p_rel_parms = &m_RelParamsInformal;
                break;
            case RelationshipType::MARITAL:
                p_rel_parms = &m_RelParamsMarital;
                break;
            case RelationshipType::COMMERCIAL:
                p_rel_parms = &m_RelParamsCommercial;
                break;
            default:
                release_assert( false );
        }

        suids::suid rel_id = GetNextSuidRelationship();
        FakeRelationship* p_rel = new FakeRelationship( pRNG, rel_id, p_rel_parms, male, female );
        p_rel->Update( duration );
        m_relationship_list.push_back( p_rel );

        return p_rel;
    }

    IndividualHumanContextFake* CreateHuman( int gender, float ageDays )
    {
        IndividualHumanInterventionsContextFake* p_hic = new IndividualHumanInterventionsContextFake();
        IndividualHumanContextFake* p_human = new IndividualHumanContextFake( p_hic, &m_NC, &m_NEC, nullptr );

        p_human->SetId( GetNextSuidHuman().data );
        p_human->SetGender( gender );
        p_human->SetAge( ageDays );

        m_hic_list.push_back( p_hic );
        m_human_list.push_back( p_human );

        return p_human;
    }

    IndividualHumanContextFake* CreateHuman( int gender, float ageDays, float percentInfected )
    {
        IndividualHumanContextFake* p_human = CreateHuman( gender, ageDays );

        p_human->SetHasSTI( m_InfectedRng.e() < percentInfected );

        return p_human;
    }

    int GetNumHumans() const
    {
        return m_human_list.size();
    }

    IndividualHumanContextFake* GetHuman( int index )
    {
        return m_human_list[ index ];
    }

    int GetNumRelationships() const
    {
        return m_relationship_list.size();
    }

    INodeContextFake* GetNodeContext()
    {
        return &m_NC;
    }

    void SetTime( float time )
    {
        m_NEC.SetTime( IdmDateTime( time ) );
    }

private:
    suids::suid GetNextSuidHuman()
    {
        suids::suid next = m_NextSuidHuman;
        m_NextSuidHuman.data++;
        return next;
    }

    suids::suid GetNextSuidRelationship()
    {
        suids::suid next = m_NextSuidRelationship;
        m_NextSuidRelationship.data++;
        return next;
    }

    INodeEventContextFake   m_NEC;
    INodeContextFake        m_NC;
    vector< Relationship* > m_relationship_list;
    vector< IndividualHumanInterventionsContextFake* > m_hic_list;
    vector< IndividualHumanContextFake*              > m_human_list;
    RelationshipParameters m_RelParamsTransitory;
    RelationshipParameters m_RelParamsInformal;
    RelationshipParameters m_RelParamsMarital;
    RelationshipParameters m_RelParamsCommercial;
    suids::suid m_NextSuidHuman;
    suids::suid m_NextSuidRelationship;
    PSEUDO_DES m_InfectedRng;
    bool m_UseDefaults;
};
