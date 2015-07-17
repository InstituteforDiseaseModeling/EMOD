/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <boost/serialization/access.hpp>
#include <list>
#include "suids.hpp"
#include "ISupports.h"
#include "IMigrate.h"
#include "Vector.h"
#include "VectorEnums.h"
#include "VectorMatingStructure.h"

namespace Kernel
{
    struct IVectorCohort : public ISupports
    {
    public:
        /* Only for type-ID right now */
        virtual int32_t GetPopulation() const = 0;
        virtual void SetPopulation( int32_t new_pop ) = 0;
        virtual double GetProgress() const = 0;
        virtual void ClearProgress() = 0;
        virtual void IncreaseProgress( double delta ) = 0;
        virtual VectorMatingStructure& GetVectorGenetics() = 0;
        virtual void SetVectorGenetics( const VectorMatingStructure& new_value ) = 0;
        virtual float GetMortality( uint32_t addition ) const = 0;
    };

    class Node;
    class StrainIdentity;

    struct VectorCohort : public IVectorCohort, public IMigrate
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorCohort *CreateCohort( float progress = 0.0, uint32_t population = DEFAULT_VECTOR_COHORT_SIZE, VectorMatingStructure = VectorMatingStructure() );
        virtual ~VectorCohort();

        virtual const StrainIdentity* GetStrainIdentity() const;

        // IMigrate interfaces
        virtual void ImmigrateTo(Node* destination_node);
        virtual void SetMigrationDestination(suids::suid destination);
        virtual const suids::suid & GetMigrationDestination();

        virtual int32_t GetPopulation() const; // used by VPI
        virtual void SetPopulation( int32_t new_pop ); // used by VPI (1x besides ClearPop)
        virtual double GetProgress() const; // NOT used by VPI
        virtual void ClearProgress(); // NOT used by VPI, implicit
        virtual void IncreaseProgress( double delta ); // used by VPI (2x)
        virtual VectorMatingStructure& GetVectorGenetics(); // used by VPI
        virtual void SetVectorGenetics( const VectorMatingStructure& new_value );
        virtual float GetMortality( uint32_t addition ) const;

    protected:
        VectorCohort();
        VectorCohort(float progress, uint32_t population, VectorMatingStructure _vector_genetics);
        void Initialize();

        VectorMatingStructure vector_genetics;
        double progress;
        int32_t population;

    private:

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, VectorCohort &obj, const unsigned int  file_version );
#endif
    };

    typedef std::list<VectorCohort *> VectorCohortList_t;
}
