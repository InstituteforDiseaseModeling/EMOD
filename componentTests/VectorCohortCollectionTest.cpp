
#include "stdafx.h"
#include "UnitTest++.h"
#include <string>
#include <memory>

#include "componentTests.h"
#include "INodeContextFake.h"
#include "INodeEventContextFake.h"
#include "VectorCohortCollection.h"
#include "VectorCohort.h"
#include "VectorContexts.h"

using namespace Kernel;

SUITE( VectorCohortCollectionTest )
{
    class INodeVectorFake : public INodeVector
    {
    public:
        INodeVectorFake()
            : INodeVector()
            , m_NextVectorID(0)
        {
        }

        ~INodeVectorFake()
        {
        }

        // ---------------------
        // --- ISupport Methods
        // ---------------------
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject )
        {
            *ppvObject = nullptr;
            if( iid == GET_IID( INodeVector ) )
                *ppvObject = static_cast<INodeVector*>(this);

            if( *ppvObject != nullptr )
            {
                return QueryResult::s_OK;
            }
            else
                return QueryResult::e_NOINTERFACE;
        }

        virtual int32_t AddRef()
        {
            return 10;
        }
        virtual int32_t Release()
        {
            return 10;
        }

        // -----------------------
        // --- INodeVector methods
        // -----------------------
        virtual suids::suid GetNextVectorSuid() override
        {
            suids::suid id;
            id.data = ++m_NextVectorID;
            return id;
        }

        virtual const VectorPopulationReportingList_t& GetVectorPopulationReporting() const
        {
            throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
        };

        virtual void AddVectors( const std::string& releasedSpecies,
                                 const VectorGenome& rGenome,
                                 const VectorGenome& rMateGenome,
                                 bool isFraction,
                                 uint32_t releasedNumber,
                                 float releasedFraction,
                                 float releasedInfectious ) override
        {
            throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
        };

        virtual void processImmigratingVector( IVectorCohort* immigrant )
        {
            throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
        };

        virtual void DepositFromIndividual( const IStrainIdentity& strain_IDs,
                                            const GeneticProbability& contagion_quantity,
                                            TransmissionRoute::Enum route ) override
        {
            throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
        };

        virtual void ExposeVector( IInfectable* candidate, float dt ) override
        {
            throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
        }

        virtual GeneticProbability GetTotalContagionGP( TransmissionRoute::Enum route ) const override
        {
            throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented." );
        };

        virtual void AddVectorPopulationToNode(IVectorPopulation* vp) override
        {
            throw Kernel::NotYetImplementedException(__FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
        }

    private:
        uint32_t m_NextVectorID;
    };

    struct VectorCohortCollectionFixture
    {
        INodeVectorFake m_NodeVector;

        VectorCohortCollectionFixture()
            : m_NodeVector()
        {
            Environment::Finalize();
            Environment::setLogger( new SimpleLogger( Logger::tLevel::WARNING ) );
        }

        ~VectorCohortCollectionFixture()
        {
            Environment::Finalize();
        }
    };

    void TestCollectionHelper( INodeVectorFake* pNodeVector,
                               VectorCohortCollectionAbstract& rCollection )
    {
        rCollection.initialize( pNodeVector, true );
        CHECK_EQUAL( 0, rCollection.size() );

        VectorCohortCollectionAbstract::Iterator it_begin = rCollection.begin();
        VectorCohortCollectionAbstract::Iterator it_end   = rCollection.end();
        CHECK( it_begin == it_end );

        VectorGenome genome0;
        VectorGenome genome1;
        genome1.SetLocus( 0, 0, 0 );
        genome1.SetLocus( 1, 1, 0 );
        genome1.SetLocus( 2, 0, 1 );
        genome1.SetLocus( 3, 1, 1 );

        VectorGenome genome2;
        genome2.SetLocus( 0, 0, 1 );
        genome2.SetLocus( 1, 1, 2 );
        genome2.SetLocus( 2, 2, 1 );
        genome2.SetLocus( 3, 1, 3 );

        VectorGenome genome3;
        genome3.SetLocus( 0, 0, 0 );
        genome3.SetLocus( 1, 1, 0 );
        genome3.SetLocus( 2, 0, 1 );
        genome3.SetLocus( 3, 3, 1 );

        CHECK( genome0 < genome1 );
        CHECK( genome1 < genome2 );
        CHECK( genome2 < genome3 );

        GenomeKey key_1( genome1, genome0 );
        CHECK( !(key_1 < key_1) );

        GenomeKey key_1_b( genome1, genome0 );
        CHECK( !(key_1_b < key_1_b) );
        CHECK( !(key_1   < key_1_b) );
        CHECK( !(key_1_b < key_1) );

        GenomeKey key_2( genome2, genome0 );
        CHECK( key_1 < key_2 );
        CHECK( !(key_2 < key_1) );

        IVectorCohort* p_vc1_a = VectorCohort::CreateCohort( 1,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.0,
                                                             0.0,
                                                             10,
                                                             genome1,
                                                             0 );

        IVectorCohort* p_vc1_b = VectorCohort::CreateCohort( 2,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.0,
                                                             0.0,
                                                             20,
                                                             genome1,
                                                             0 );

        IVectorCohort* p_vc2_a = VectorCohort::CreateCohort( 3,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.0,
                                                             0.0,
                                                             30,
                                                             genome2,
                                                             0 );

        IVectorCohort* p_vc2_b = VectorCohort::CreateCohort( 4,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.0,
                                                             0.0,
                                                             40,
                                                             genome2,
                                                             0 );


        IVectorCohort* p_vc3_a = VectorCohort::CreateCohort( 5,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.0,
                                                             0.0,
                                                             50,
                                                             genome3,
                                                             0 );

        IVectorCohort* p_vc3_b = VectorCohort::CreateCohort( 6,
                                                             VectorStateEnum::STATE_ADULT,
                                                             1.0,
                                                             0.0,
                                                             0.0,
                                                             60,
                                                             genome3,
                                                             0 );

        // --------------
        // --- Add First
        // --------------
        rCollection.add( p_vc1_a, 0.0, true );
        CHECK_EQUAL( 1, rCollection.size() );

        it_begin = rCollection.begin();
        it_end = rCollection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // --------------
        // --- Add second
        // --------------
        rCollection.add( p_vc2_a, 0.0, true );
        CHECK_EQUAL( 2, rCollection.size() );

        it_begin = rCollection.begin();
        it_end   = rCollection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // --------------
        // --- Add Third
        // --------------
        rCollection.add( p_vc3_a, 0.0, true );
        CHECK_EQUAL( 3, rCollection.size() );

        it_begin = rCollection.begin();
        it_end   = rCollection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        CHECK( p_vc1_a->GetPopulation() == 10 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        CHECK( p_vc2_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -----------------------------------------------
        // --- Add Forth but assume merged with first one
        // -----------------------------------------------
        rCollection.add( p_vc1_b, 0.0, true );
        CHECK_EQUAL( 3, rCollection.size() );

        it_begin = rCollection.begin();
        it_end   = rCollection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        CHECK( p_vc1_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        CHECK( p_vc2_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -----------------------------------------------
        // --- Add Fifth but assume merged with second one
        // -----------------------------------------------
        rCollection.add( p_vc2_b, 0.0, true );
        CHECK_EQUAL( 3, rCollection.size() );

        it_begin = rCollection.begin();
        it_end   = rCollection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        CHECK( p_vc1_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        CHECK( p_vc2_a->GetPopulation() == 70 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        CHECK( p_vc3_a->GetPopulation() == 50 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -----------------------------------------------
        // --- Add Sixth but assume did not merge
        // -----------------------------------------------
        rCollection.add( p_vc3_b, 0.0, true );
        CHECK_EQUAL( 4, rCollection.size() );

        it_begin = rCollection.begin();
        it_end   = rCollection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        CHECK( p_vc1_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        CHECK( p_vc2_a->GetPopulation() == 70 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        CHECK( p_vc3_a->GetPopulation() == 50 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_b );
        CHECK( p_vc3_b->GetPopulation() == 60 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------------
        // --- Test Removing
        // -------------------
        it_begin = rCollection.begin();
        CHECK( *it_begin == p_vc1_a );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        rCollection.remove( it_begin );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_b );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------------
        // --- Test Removing another
        // -------------------
        it_begin = rCollection.begin();
        CHECK( *it_begin == p_vc1_a );
        rCollection.remove( it_begin );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_b );
        ++it_begin;
        CHECK( it_begin == it_end );

        rCollection.clear();
    }

    TEST_FIXTURE( VectorCohortCollectionFixture, TestCollectionWithAging )
    {
        VectorCohortCollectionStdMapWithAging collection;
        TestCollectionHelper( &m_NodeVector, collection );
    }

    TEST_FIXTURE( VectorCohortCollectionFixture, TestCollectionWithProgress )
    {
        VectorCohortCollectionStdMapWithProgress collection;
        TestCollectionHelper( &m_NodeVector, collection );
    }

    TEST_FIXTURE( VectorCohortCollectionFixture, TestCollectionWithProgressAgingOff )
    {
        VectorCohortCollectionStdMapWithProgress collection;

        collection.initialize( &m_NodeVector, false );
        CHECK_EQUAL( 0, collection.size() );

        VectorCohortCollectionAbstract::Iterator it_begin = collection.begin();
        VectorCohortCollectionAbstract::Iterator it_end = collection.end();
        CHECK( it_begin == it_end );

        VectorGenome genome1;
        genome1.SetLocus( 0, 0, 0 );

        IVectorCohort* p_vc1_a = VectorCohort::CreateCohort( 1,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.0,
                                                             0.0,
                                                             10,
                                                             genome1,
                                                             0 );

        IVectorCohort* p_vc1_b = VectorCohort::CreateCohort( 2,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.0,
                                                             0.0,
                                                             20,
                                                             genome1,
                                                             0 );

        IVectorCohort* p_vc2_a = VectorCohort::CreateCohort( 3,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.25,
                                                             0.0,
                                                             30,
                                                             genome1,
                                                             0 );

        IVectorCohort* p_vc2_b = VectorCohort::CreateCohort( 4,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.25,
                                                             0.0,
                                                             40,
                                                             genome1,
                                                             0 );


        IVectorCohort* p_vc3_a = VectorCohort::CreateCohort( 5,
                                                             VectorStateEnum::STATE_ADULT,
                                                             0.0,
                                                             0.5,
                                                             0.0,
                                                             50,
                                                             genome1,
                                                             0 );

        IVectorCohort* p_vc3_b = VectorCohort::CreateCohort( 6,
                                                             VectorStateEnum::STATE_ADULT,
                                                             1.0,
                                                             0.5,
                                                             0.0,
                                                             60,
                                                             genome1,
                                                             0 );

        // --------------
        // --- Add First
        // --------------
        collection.add( p_vc1_a, 0.1f, true );
        CHECK_EQUAL( 1, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // --------------
        // --- Add second - should not merge because progress is different
        // --------------
        collection.add( p_vc2_a, 0.1f, true );
        CHECK_EQUAL( 2, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // --------------
        // --- Add Third - should not merge because progress is different
        // --------------
        collection.add( p_vc3_a, 0.1f, true );
        CHECK_EQUAL( 3, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        CHECK( p_vc1_a->GetPopulation() == 10 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        CHECK( p_vc2_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -----------------------------------------------
        // --- Add Forth but assume merged with first one
        // -----------------------------------------------
        collection.add( p_vc1_b, 0.1f, true );
        CHECK_EQUAL( 3, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        CHECK( p_vc1_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        CHECK( p_vc2_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -----------------------------------------------
        // --- Add Fifth but assume merged with second one
        // -----------------------------------------------
        collection.add( p_vc2_b, 0.1f, true );
        CHECK_EQUAL( 3, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        CHECK( p_vc1_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        CHECK( p_vc2_a->GetPopulation() == 70 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        CHECK( p_vc3_a->GetPopulation() == 50 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -----------------------------------------------
        // --- Add Sixth but assume did not merge becaue age is different
        // -----------------------------------------------
        collection.add( p_vc3_b, 0.1f, true );
        CHECK_EQUAL( 4, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1_a );
        CHECK( p_vc1_a->GetPopulation() == 30 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        CHECK( p_vc2_a->GetPopulation() == 70 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        CHECK( p_vc3_a->GetPopulation() == 50 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_b );
        CHECK( p_vc3_b->GetPopulation() == 60 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------------
        // --- Test Removing
        // -------------------
        it_begin = collection.begin();
        CHECK( *it_begin == p_vc1_a );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2_a );
        collection.remove( it_begin );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_b );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------------
        // --- Test Removing another
        // -------------------
        it_begin = collection.begin();
        CHECK( *it_begin == p_vc1_a );
        collection.remove( it_begin );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_a );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3_b );
        ++it_begin;
        CHECK( it_begin == it_end );

        collection.clear();
    }

    TEST_FIXTURE( VectorCohortCollectionFixture, TestCollectionStdVector )
    {
        VectorCohortCollectionStdVector collection;
        collection.initialize( &m_NodeVector, true );
        CHECK_EQUAL( 0, collection.size() );

        VectorCohortCollectionAbstract::Iterator it_begin = collection.begin();
        VectorCohortCollectionAbstract::Iterator it_end = collection.end();
        CHECK( it_begin == it_end );

        VectorGenome genome0;
        VectorGenome genome1;
        genome1.SetLocus( 0, 0, 0 );
        genome1.SetLocus( 1, 1, 0 );
        genome1.SetLocus( 2, 0, 1 );
        genome1.SetLocus( 3, 1, 1 );

        VectorGenome genome2;
        genome2.SetLocus( 0, 0, 1 );
        genome2.SetLocus( 1, 1, 2 );
        genome2.SetLocus( 2, 2, 1 );
        genome2.SetLocus( 3, 1, 3 );

        VectorGenome genome3;
        genome3.SetLocus( 0, 0, 0 );
        genome3.SetLocus( 1, 1, 0 );
        genome3.SetLocus( 2, 0, 1 );
        genome3.SetLocus( 3, 3, 1 );

        CHECK( genome0 < genome1 );
        CHECK( genome1 < genome2 );
        CHECK( genome2 < genome3 );

        GenomeKey key_1( genome1, genome0 );
        CHECK( !(key_1 < key_1) );

        GenomeKey key_1_b( genome1, genome0 );
        CHECK( !(key_1_b < key_1_b) );
        CHECK( !(key_1   < key_1_b) );
        CHECK( !(key_1_b < key_1) );

        GenomeKey key_2( genome2, genome0 );
        CHECK( key_1 < key_2 );
        CHECK( !(key_2 < key_1) );

        IVectorCohort* p_vc1 = VectorCohort::CreateCohort( 1,
                                                           VectorStateEnum::STATE_ADULT,
                                                           0.0,
                                                           0.0,
                                                           0.0,
                                                           1,
                                                           genome1,
                                                           0 );

        IVectorCohort* p_vc2 = VectorCohort::CreateCohort( 2,
                                                           VectorStateEnum::STATE_ADULT,
                                                           0.0,
                                                           0.0,
                                                           0.0,
                                                           1,
                                                           genome1,
                                                           0 );

        IVectorCohort* p_vc3 = VectorCohort::CreateCohort( 3,
                                                           VectorStateEnum::STATE_ADULT,
                                                           0.0,
                                                           0.0,
                                                           0.0,
                                                           1,
                                                           genome2,
                                                           0 );

        IVectorCohort* p_vc4 = VectorCohort::CreateCohort( 4,
                                                           VectorStateEnum::STATE_ADULT,
                                                           0.0,
                                                           0.0,
                                                           0.0,
                                                           1,
                                                           genome2,
                                                           0 );


        IVectorCohort* p_vc5 = VectorCohort::CreateCohort( 5,
                                                           VectorStateEnum::STATE_ADULT,
                                                           0.0,
                                                           0.0,
                                                           0.0,
                                                           1,
                                                           genome3,
                                                           0 );

        IVectorCohort* p_vc6 = VectorCohort::CreateCohort( 6,
                                                           VectorStateEnum::STATE_ADULT,
                                                           1.0,
                                                           0.0,
                                                           0.0,
                                                           1,
                                                           genome3,
                                                           0 );

        // -------------
        // --- Add First
        // -------------
        collection.add( p_vc1, 0.0, true );
        CHECK_EQUAL( 1, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------
        // --- Add Second
        // -------------
        collection.add( p_vc2, 0.0, true );
        CHECK_EQUAL( 2, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------
        // --- Add Third
        // -------------
        collection.add( p_vc3, 0.0, true );
        CHECK_EQUAL( 3, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------
        // --- Add Forth
        // -------------
        collection.add( p_vc4, 0.0, true );
        CHECK_EQUAL( 4, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc4 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------
        // --- Add Fifth
        // -------------
        collection.add( p_vc5, 0.0, true );
        CHECK_EQUAL( 5, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc4 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc5 );
        ++it_begin;
        CHECK( it_begin == it_end );

        // -------------
        // --- Add Sixth
        // -------------
        collection.add( p_vc6, 0.0, true );
        CHECK_EQUAL( 6, collection.size() );

        it_begin = collection.begin();
        it_end = collection.end();

        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc1 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc4 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc5 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc6 );
        ++it_begin;
        CHECK( it_begin == it_end );
        CHECK_EQUAL( 6, collection.size() );

        // -----------------
        // --- Test removing
        // -----------------
        it_begin = collection.begin();
        CHECK( *it_begin == p_vc1 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc2 );
        collection.remove( it_begin ); // removing #2
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc6 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc4 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc5 );
        ++it_begin;
        CHECK( it_begin == it_end );
        CHECK_EQUAL( 5, collection.size() );

        // ------------------
        // --- remove another
        // ------------------
        it_begin = collection.begin();
        CHECK( *it_begin == p_vc1 );
        collection.remove( it_begin );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc5 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc6 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc3 );
        ++it_begin;
        CHECK( it_begin != it_end );
        CHECK( *it_begin == p_vc4 );
        ++it_begin;
        CHECK( it_begin == it_end );
        CHECK_EQUAL( 4, collection.size() );

        collection.clear();
    }

    void TestIterationHelper( INodeVectorFake* pNodeVector,
                              std::vector<VectorGenome>& rFemaleGenomes,
                              std::vector<VectorGenome>& rMaleGenomes,
                              VectorCohortCollectionAbstract& rCollection )
    {
        // -------------------------------------------------------------------------
        // --- Create a set of gamets where there are two genes and two alleles each
        // -------------------------------------------------------------------------
        std::vector<VectorGamete> gamete_list;
        gamete_list.push_back( VectorGamete() );
        for( int locus = 1; locus <= 2; ++locus )
        {
            std::vector<VectorGamete> tmp_list = gamete_list;
            gamete_list.clear();
            for( int allele_index = 0; allele_index <= 1; ++allele_index )
            {
                for( auto g : tmp_list )
                {
                    g.SetLocus( locus, allele_index );
                    gamete_list.push_back( g );
                }
            }
        }
        CHECK_EQUAL( 4, gamete_list.size() );

        // ---------------------------------------------------------------------
        // --- Create a set of genomes that is a combination of these gametes
        // ---------------------------------------------------------------------
        for( auto mom : gamete_list )
        {
            for( auto dad : gamete_list )
            {
                rFemaleGenomes.push_back( VectorGenome( mom, dad ) );
            }
        }
        CHECK_EQUAL( 16, rFemaleGenomes.size() );

        // --------------------------------
        // --- Create a set of male genomes
        // --------------------------------
        for( auto g : rFemaleGenomes )
        {
            g.SetLocus( 0, 0, 1 ); // make genome male by setting Y
            rMaleGenomes.push_back( g );
        }
        CHECK_EQUAL( 16, rMaleGenomes.size() );

        // ----------------------------------------------
        // --- Create a collection of vectors where each
        // --- female genome mates with each male genome
        // ----------------------------------------------
        rCollection.initialize( pNodeVector, true );
        CHECK_EQUAL( 0, rCollection.size() );

        for( auto female : rFemaleGenomes )
        {
            for( auto male : rMaleGenomes )
            {
                IVectorCohort* pvc = VectorCohort::CreateCohort( pNodeVector->GetNextVectorSuid().data,
                                                                 VectorStateEnum::STATE_ADULT,
                                                                 0.0,
                                                                 0.0,
                                                                 0.0,
                                                                 10,
                                                                 female,
                                                                 0 );
                pvc->SetMateGenome( male );
                rCollection.add( pvc, 0.0, true );
            }
        }
        CHECK_EQUAL( 256, rCollection.size() );

        // -------------------------------------------------------------
        // --- Verify that iterating through exposes you to each cohort
        // -------------------------------------------------------------
        int counter = 0;
        uint32_t population = 0;
        for( auto pvc : rCollection )
        {
            ++counter;
            population += pvc->GetPopulation();
        }
        CHECK_EQUAL( 256, counter );
        CHECK_EQUAL( 2560, population );

        // ----------------------------------------------------------------------
        // --- Remove some of the cohorts while iterating through the collection.
        // ----------------------------------------------------------------------
        counter = 0;
        population = 0;
        for( auto it = rCollection.begin(); it != rCollection.end(); ++it )
        {
            IVectorCohort* pvc = *it;
            ++counter;

            if( (counter == 1) || (counter == 32) || (counter == 40) ||
                (counter == 49) || (counter == 80) || (counter == 88) )
            {
                rCollection.remove( it );
                delete pvc;
            }
            else
            {
                population += pvc->GetPopulation();
            }
        }
        CHECK_EQUAL( 256, counter );
        CHECK_EQUAL( 2500, population );

        // -------------------------------------------------
        // --- Verify that iterating through the collection
        // --- exposes you to what remains in the list
        // -------------------------------------------------
        counter = 0;
        population = 0;
        for( auto pvc : rCollection )
        {
            ++counter;
            population += pvc->GetPopulation();
        }
        CHECK_EQUAL( 250, counter );
        CHECK_EQUAL( 2500, population );
    }

    void TestInterationStdMapHelper( INodeVectorFake* pNodeVector,
                                     std::vector<VectorGenome>& rFemaleGenomes,
                                     std::vector<VectorGenome>& rMaleGenomes,
                                     VectorCohortCollectionAbstract& rCollection )
    {

        // ------------------------------------------------------------
        // --- Add again a similar set of cohorts as the initial one.
        // --- This time we expect most of these cohorts to get merged
        // --- with existing cohortsm, but the ones that were removed
        // --- should be added.
        // ------------------------------------------------------------
        for( auto female : rFemaleGenomes )
        {
            for( auto male : rMaleGenomes )
            {
                IVectorCohort* pvc = VectorCohort::CreateCohort( pNodeVector->GetNextVectorSuid().data,
                                                                 VectorStateEnum::STATE_ADULT,
                                                                 0.0,
                                                                 0.0,
                                                                 0.0,
                                                                 10,
                                                                 female,
                                                                 0 );
                pvc->SetMateGenome( male );
                rCollection.add( pvc, 0.0, true );
            }
        }
        CHECK_EQUAL( 256, rCollection.size() );

        // ----------------
        // --- Verify it
        // ----------------
        uint32_t counter = 0;
        uint32_t population = 0;
        for( auto pvc : rCollection )
        {
            ++counter;
            population += pvc->GetPopulation();
        }
        CHECK_EQUAL( 256, counter );
        CHECK_EQUAL( 5060, population );

        // --------------------------------------------------------
        // --- add the vectors again but this time change their AGE
        // --------------------------------------------------------
        for( auto female : rFemaleGenomes )
        {
            for( auto male : rMaleGenomes )
            {
                IVectorCohort* pvc = VectorCohort::CreateCohort( pNodeVector->GetNextVectorSuid().data,
                                                                 VectorStateEnum::STATE_ADULT,
                                                                 5.0,
                                                                 0.0,
                                                                 0.0,
                                                                 10,
                                                                 female,
                                                                 0 );
                pvc->SetMateGenome( male );
                rCollection.add( pvc, 0.0, true );
            }
        }
        CHECK_EQUAL( 512, rCollection.size() );

        // ----------------
        // --- Verify it
        // ----------------
        counter = 0;
        population = 0;
        for( auto pvc : rCollection )
        {
            ++counter;
            population += pvc->GetPopulation();
        }
        CHECK_EQUAL( 512, counter );
        CHECK_EQUAL( 7620, population );
    }

    TEST_FIXTURE( VectorCohortCollectionFixture, TestIterationWithAging )
    {
        VectorCohortCollectionStdMapWithAging collection;

        std::vector<VectorGenome> female_genomes;
        std::vector<VectorGenome> male_genomes;
        TestIterationHelper(        &m_NodeVector, female_genomes, male_genomes, collection );
        TestInterationStdMapHelper( &m_NodeVector, female_genomes, male_genomes, collection );
    }

    TEST_FIXTURE( VectorCohortCollectionFixture, TestIterationWithProgress )
    {
        VectorCohortCollectionStdMapWithProgress collection;

        std::vector<VectorGenome> female_genomes;
        std::vector<VectorGenome> male_genomes;
        TestIterationHelper(        &m_NodeVector,  female_genomes, male_genomes, collection );
        TestInterationStdMapHelper( &m_NodeVector,  female_genomes, male_genomes, collection );
    }

    TEST_FIXTURE( VectorCohortCollectionFixture, TestIterationStdVector )
    {
        VectorCohortCollectionStdVector collection;

        std::vector<VectorGenome> female_genomes;
        std::vector<VectorGenome> male_genomes;
        TestIterationHelper( &m_NodeVector, female_genomes, male_genomes, collection );
    }
}