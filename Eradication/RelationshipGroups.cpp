
#include "stdafx.h"

#include "RelationshipGroups.h"
#include "Relationship.h"
#include "INodeSTI.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "INodeContext.h"
#include "RANDOM.h"
#include "Debug.h"

SETUP_LOGGING( "RelationshipGroups" )

namespace Kernel {

#ifndef DISABLE_STI

    // ------------------------------------------------------------------------
    // --- DiscreteContagionPopulation
    // ------------------------------------------------------------------------

    BEGIN_QUERY_INTERFACE_BODY(DiscreteContagionPopulation)
        HANDLE_INTERFACE(IContagionPopulation)
        HANDLE_INTERFACE(IContagionInfo)
    END_QUERY_INTERFACE_BODY(DiscreteContagionPopulation)

    DiscreteContagionPopulation:: DiscreteContagionPopulation( const CoitalActAndStrain& rCoitalActAndStrain,
                                                               float interventionReducedAcquire )
        : m_CoitalActAndStrain( rCoitalActAndStrain )
        , m_InterventionReducedAcquire( interventionReducedAcquire )
    {
    }

    DiscreteContagionPopulation:: ~DiscreteContagionPopulation()
    {
    }

    int DiscreteContagionPopulation::GetAntigenID( void ) const
    {
        return m_CoitalActAndStrain.strain.GetAntigenID();
    }

    float DiscreteContagionPopulation::GetTotalContagion( void ) const
    {
        return 0;
    }

    bool DiscreteContagionPopulation::ResolveInfectingStrain( IStrainIdentity* strainId ) const
    {
        strainId->SetAntigenID( GetAntigenID() );
        strainId->SetGeneticID( m_CoitalActAndStrain.strain.GetGeneticID() );

        return true;
    }

    const CoitalAct& DiscreteContagionPopulation::GetCoitalAct() const
    {
        return m_CoitalActAndStrain.coital_act;
    }

    float DiscreteContagionPopulation::GetInterventionReducedAcquire() const
    {
        return m_InterventionReducedAcquire;
    }

    // ------------------------------------------------------------------------
    // --- RelationshipGroups
    // ------------------------------------------------------------------------

    RelationshipGroups::RelationshipGroups()
        : m_ExposedPersonsCoitalActsMap()
        , m_parent( nullptr )
    {
    }

    void RelationshipGroups::AddProperty( const string& property,
                                          const PropertyValueList_t& values,
                                          const ScalingMatrix_t& scalingMatrix )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "not supported in STI" );
    }

    void RelationshipGroups::GetGroupMembershipForProperties( const IPKeyValueContainer& properties,
                                                              TransmissionGroupMembership_t& membershipOut ) const
    {
        // do nothing
    }

    void RelationshipGroups::Build(
        float contagionDecayRate,
        int numberOfStrains,
        int numberOfSubstrains )
    {
        m_ExposedPersonsCoitalActsMap.clear();
    }

    void RelationshipGroups::UpdatePopulationSize( const TransmissionGroupMembership_t& transmissionGroupMembership,
                                                   float size_changes,
                                                   float mc_weight )
    {
        // we don't do anything with population size
    }

    void RelationshipGroups::ExposeToContagion(
        IInfectable* candidate,
        TransmissionGroupMembership_t poolMembership,
        float deltaTee,
        TransmissionRoute::Enum tx_route )
    {
        release_assert( candidate != nullptr );

        uint32_t exposed_human_id = candidate->GetSuid().data;
        const std::map<uint32_t,std::vector<CoitalActAndStrain>>& r_exposed_person_rels = m_ExposedPersonsCoitalActsMap.at( exposed_human_id );

        struct RelIdAndRelIndex
        {
            uint32_t rel_id;
            uint32_t rel_index;
        };
        std::vector<RelIdAndRelIndex> indexes;
        int rel_index = 0;
        for( auto& it : r_exposed_person_rels )
        {
            for( int i = 0; i < it.second.size(); ++i )
            {
                RelIdAndRelIndex rel_id_index;
                rel_id_index.rel_id = it.first;
                rel_id_index.rel_index = rel_index;
                indexes.push_back( rel_id_index );
            }
            ++rel_index;
        }
        std::vector<int> act_index_per_rel( r_exposed_person_rels.size(), 0 );

        if( r_exposed_person_rels.size() > 1 )
        {
            // Randomly shuffle the entries of the vector
            auto myran = [ this ]( int i ) { return this->m_parent->GetRng()->uniformZeroToN32( i ); };
            std::random_shuffle( indexes.begin(), indexes.end(), myran );
        }

        for( const RelIdAndRelIndex& r_id_index : indexes )
        {
            int act_index = act_index_per_rel[ r_id_index.rel_index ];

            const CoitalActAndStrain& r_act_strain = r_exposed_person_rels.at( r_id_index.rel_id )[ act_index ];

            release_assert( exposed_human_id == r_act_strain.coital_act.GetUnInfectedPartnerID().data );

            DiscreteContagionPopulation contagionPopulation( r_act_strain, candidate->GetInterventionReducedAcquire() );
            candidate->Expose( &contagionPopulation, deltaTee );

            act_index_per_rel[ r_id_index.rel_index ] += 1;
        }
    }

    float RelationshipGroups::GetTotalContagion()
    {
        return 0;
    }

    void RelationshipGroups::DepositContagion(
        const IStrainIdentity& strain,
        float prob,
        TransmissionGroupMembership_t poolMembership )
    {
        throw new IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "use the other DepositContagion method" );
    }

    void RelationshipGroups::UseTotalPopulationForNormalization( void )
    {
        throw new IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be used" );
    }

    void RelationshipGroups::UseGroupPopulationForNormalization( void )
    {
        throw new IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be used" );
    }

    float RelationshipGroups::GetPopulationSize( const TransmissionGroupMembership_t& transmissionGroupMembership ) const
    {
        throw new IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be used" );
    }

    void RelationshipGroups::ClearPopulationSize()
    {
        // do nothing
    }

    void RelationshipGroups::SetTag( const std::string& tag )
    {
        throw new IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be used" );
    }

    const std::string& RelationshipGroups::GetTag( void ) const
    {
        throw new IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be used" );
    }

    void RelationshipGroups::ClearStrain( const IStrainIdentity* pStrain, const TransmissionGroupMembership_t& membership )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "not supported in STI" );
    }

    float RelationshipGroups::GetTotalContagionForGroup( TransmissionGroupMembership_t membership )
    {
        throw new IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be used" );
    }

    void RelationshipGroups::CorrectInfectivityByGroup( float infectivityCorrection,
                                                        TransmissionGroupMembership_t transmissionGroupMembership )
    {
        // do nothing
    }

    void RelationshipGroups::EndUpdate( float infectivityMultiplication, float infectivityAddition )
    {
        // do nothing
    }

    float RelationshipGroups::GetContagionByProperty( const IPKeyValue& property_value )
    {
        throw new IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be used" );

    }

    void RelationshipGroups::SetParent( INodeSTI * parent )
    {
        m_parent = parent;
    }

    void RelationshipGroups::AddProperty(
        const string& property,
        const std::list<uint32_t>& values,
        const ScalingMatrix_t& scalingMatrix )
    {
        // do nothing
    }

    void RelationshipGroups::addPropertyValuesToValueToIndexMap(
        const string& propertyName,
        const std::list<uint32_t>& valueSet,
        int currentMatrixSize )
    {
        // do nothing
    }

    void RelationshipGroups::DepositContagion( const IStrainIdentity& strain, const CoitalAct& rCoitalAct )
    {
        std::vector<CoitalActAndStrain>& r_exposed_persons_coital_acts_per_rel = m_ExposedPersonsCoitalActsMap[ rCoitalAct.GetUnInfectedPartnerID().data ][ rCoitalAct.GetRelationshipID().data ];
        CoitalActAndStrain act( rCoitalAct, strain );
        r_exposed_persons_coital_acts_per_rel.push_back( act );
    }

    void RelationshipGroups::CreateRandomIndexes( std::vector<int>& rRandomIndexes ) const
    {
        for( uint32_t index = 0; index < rRandomIndexes.size(); ++index )
        {
            rRandomIndexes[ index ] = index;
        }

        // Randomly shuffle the entries of the vector
        auto myran = [ this ]( int i ) { return this->m_parent->GetRng()->uniformZeroToN32( i ); };
        std::random_shuffle( rRandomIndexes.begin(), rRandomIndexes.end(), myran );
    }


#endif
}
