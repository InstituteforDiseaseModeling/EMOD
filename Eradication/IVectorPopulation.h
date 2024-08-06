
#pragma once

#include "IVectorCohort.h"
#include "ISerializable.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    struct INodeContext;

    struct IVectorPopulation : ISerializable
    {
        virtual const std::string& get_SpeciesID() const = 0;
        virtual void SetContextTo( INodeContext *context ) = 0;
        virtual void SetupLarvalHabitat( INodeContext *context ) = 0;
        virtual void SetVectorMortality( bool mortality ) = 0;

        // The function that NodeVector calls into once per species per timestep
        virtual void UpdateVectorPopulation( float dt ) = 0;

        // For NodeVector to calculate # of migrating vectors (processEmigratingVectors) and put them in new node (processImmigratingVector)
        virtual void SetupMigration( const std::string& idreference,
                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) = 0;
        virtual void Vector_Migration( float dt, VectorCohortVector_t* pMigratingQueue, bool migrate_males_only) = 0;
        virtual void AddImmigratingVector( IVectorCohort* pvc ) = 0;
        virtual void SetSortingVectors() = 0;
        virtual void SortImmigratingVectors() = 0;

        // Supports MosquitoRelease intervention
        virtual void AddVectors( const VectorGenome& rGenome,
                                 const VectorGenome& rMateGenome,
                                 bool isFraction,
                                 uint32_t releasedNumber,
                                 float releasedFraction,
                                 float releasedInfectious ) = 0;
    };
}
