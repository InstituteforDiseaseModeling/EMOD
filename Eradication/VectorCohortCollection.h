
#pragma once

#include <map>

#include "VectorGenome.h"
#include "IVectorCohort.h"
#include "IArchive.h"

namespace Kernel
{
    class INodeVector;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! NOTE:  It is tempting to put other state things like age or progress into this key.
    // !!! However, be careful because you can only use things that are constant.  For example,
    // !!! if you use age to put the cohort into the map, you won't be able to find the cohort
    // !!! the cohort's age is increased.  Genome works because it is the same for the entire
    // !!! life of the cohort.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    class GenomeKey
    {
    public:
        GenomeKey( const VectorGenome& rSelf, const VectorGenome& rMate )
            : m_Self( rSelf )
            , m_Mate( rMate )
        {
        };

        bool operator<( const GenomeKey& rThat ) const
        {
            if( this->m_Self < rThat.m_Self )
                return true;
            else if( this->m_Self == rThat.m_Self )
                return (this->m_Mate < rThat.m_Mate);
            else
                return false;
        };

    private:
        VectorGenome m_Self;
        VectorGenome m_Mate;
    };

    // This is the base interface of a group of collections that were created for
    // performance reasons.
    class VectorCohortCollectionAbstract
    {
    public:
        // This single iterator supports all of the collections.  This was done
        // instead of subclassing it because we wanted to avoid the virtual functions
        // in places called a lot.
        class Iterator
        {
        public:
            ~Iterator();

            bool operator==( const Iterator& rThat ) const;
            bool operator!=( const Iterator& rThat ) const;

            Iterator& operator++(); //prefix

            IVectorCohort* operator*();

        protected:
            friend class VectorCohortCollectionAbstract;
            friend class VectorCohortCollectionStdVector;
            friend class VectorCohortCollectionStdMap;
            friend class VectorCohortCollectionStdMapWithProgress;
            friend class VectorCohortCollectionStdMapWithAging;

            Iterator( const std::map<GenomeKey, std::vector<IVectorCohort*>>::const_iterator& it_begin,
                      const std::map<GenomeKey, std::vector<IVectorCohort*>>::const_iterator& it_end );

            Iterator( std::vector<IVectorCohort*>* pStdVector, size_t startIndex );

            void remove( bool usingFixedSize );

            std::map<GenomeKey, std::vector<IVectorCohort*>>::const_iterator m_MapIterator;
            std::map<GenomeKey, std::vector<IVectorCohort*>>::const_iterator m_MapIteratorEnd;
            std::vector<IVectorCohort* >* m_pCurrentVector;
            int32_t m_VectorIndex;
            bool m_IsStdVector;
        };

        VectorCohortCollectionAbstract();
        virtual ~VectorCohortCollectionAbstract();

        virtual void initialize( INodeVector* pNodeVector, bool agingEnabled );
        virtual Iterator begin() const = 0;
        virtual Iterator end() const = 0;
        virtual size_t size() const = 0;
        virtual void clear() = 0;

        virtual void add( IVectorCohort* pCohort, float progressThisTimestep, bool attemptMerge ) = 0;
        virtual void remove( Iterator& it ) = 0;
        virtual void compact() = 0;
        virtual void updateAge( float dt ) = 0;

        static void serialize( IArchive& ar, VectorCohortCollectionAbstract& obj );

    protected:
        INodeVector* m_pNodeVector;
        bool m_AgingEnabled;
    };

    // This collection was created for individual cohorts.  It will not attempt to
    // merge the cohorts.
    class VectorCohortCollectionStdVector : public VectorCohortCollectionAbstract
    {
    public:

        VectorCohortCollectionStdVector();
        virtual ~VectorCohortCollectionStdVector();

        virtual Iterator begin() const override;
        virtual Iterator end() const override;
        virtual size_t size() const override;
        virtual void clear() override;

        virtual void add( IVectorCohort* pCohort, float progressThisTimestep, bool attemptMerge ) override;
        virtual void remove( Iterator& it ) override;
        virtual void compact() override;
        virtual void updateAge( float dt ) override;

    private:
        std::vector<IVectorCohort*> m_StdVector;
    };

    // This is the base class for a group of collections that need an STL map to increase
    // performance when dealing with genetics.  The map is used to more quickly find the 
    // similar cohort it should merge with.
    class VectorCohortCollectionStdMap : public VectorCohortCollectionAbstract
    {
    public:

        VectorCohortCollectionStdMap();
        virtual ~VectorCohortCollectionStdMap();

        virtual Iterator begin() const override;
        virtual Iterator end() const override;
        virtual size_t size() const override;
        virtual void clear() override;

        //virtual void add( IVectorCohort* pCohort, float progressThisTimestep, bool attemptMerge ) = 0;
        //virtual void remove( Iterator& it ) = 0;
        virtual void compact() override;
        virtual void updateAge( float dt ) override;

    protected:
        std::map<GenomeKey, std::vector<IVectorCohort*>> m_MapOfVectors;
        size_t m_Size;
    };

    // This collection is meant to be used with cohorts that use "progress" instead of age
    // to determine if they should move to the next stage.  These cohorts include larva,
    // immature, and infected.  Since the amount a vector can progress in a single day is
    // variable on things like temperature, the merging process is more complicated because
    // we have to find a cohort whose progress is "simiar".
    class VectorCohortCollectionStdMapWithProgress : public VectorCohortCollectionStdMap
    {
    public:

        VectorCohortCollectionStdMapWithProgress();
        virtual ~VectorCohortCollectionStdMapWithProgress();

        virtual void add( IVectorCohort* pCohort, float progressThisTimestep, bool attemptMerge ) override;
        virtual void remove( Iterator& it ) override;

    private:
    };

    // This collection was created for the adults in the cohort model.  The big motivator was
    // immigrating vectors.  If there are lots of cohort immigrating, then we can spend a lot
    // of time searching for the cohort to merge with.  This collection reduces the search by 
    // maintaining arrays where each index represents a particular age.  That is, instead of
    // searching to find the cohort to merge with, you translate the cohort's age into an
    // index and use that to find the a possible merge candidate in the array.
    class VectorCohortCollectionStdMapWithAging : public VectorCohortCollectionStdMap
    {
    public:

        VectorCohortCollectionStdMapWithAging();
        virtual ~VectorCohortCollectionStdMapWithAging();

        virtual void add( IVectorCohort* pCohort, float progressThisTimestep, bool attemptMerge ) override;
        virtual void remove( Iterator& it ) override;
        virtual void updateAge( float dt ) override;

    private:
    };
}