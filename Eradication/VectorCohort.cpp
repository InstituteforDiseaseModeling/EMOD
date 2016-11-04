/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "VectorCohort.h"
#include "Exceptions.h"
#include "INodeContext.h"
#include "StrainIdentity.h"

static const char * _module = "VectorCohort";

namespace Kernel
{
    // QI stuff
    BEGIN_QUERY_INTERFACE_BODY(VectorCohort)
        HANDLE_INTERFACE(IVectorCohort)
        HANDLE_INTERFACE(IMigrate)
        HANDLE_ISUPPORTS_VIA(IVectorCohort)
    END_QUERY_INTERFACE_BODY(VectorCohort)

    VectorCohort::VectorCohort() 
        : vector_genetics( VectorMatingStructure() )
        , progress(0.0)
        , population(DEFAULT_VECTOR_COHORT_SIZE)
    {
    }

    VectorCohort::VectorCohort(float _progress, uint32_t _population, VectorMatingStructure _vector_genetics)
        : vector_genetics(_vector_genetics)
        , progress(_progress)
        , population(_population)
    {
    }

    void VectorCohort::Initialize()
    {
    }

    VectorCohort *VectorCohort::CreateCohort(float progress, uint32_t population, VectorMatingStructure vector_genetics)
    {
        VectorCohort *newqueue = _new_ VectorCohort(progress, population, vector_genetics);
        newqueue->Initialize();

        return newqueue;
    }

    const StrainIdentity* VectorCohort::GetStrainIdentity() const
    {
        // dummy strain identity for cohort model
        // derived VectorCohortIndividual will actually keep track of strains
        // memory is freed after contagion is queued in VectorPopulation::ProcessFeedingCycle
        return _new_ StrainIdentity();
    }

    void VectorCohort::ImmigrateTo(INodeContext* destination_node)
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Vector migration only currently supported for individual (not cohort) model." );
    }

    void VectorCohort::SetMigrating( suids::suid destination, 
                                     MigrationType::Enum type, 
                                     float timeUntilTrip, 
                                     float timeAtDestination,
                                     bool isDestinationNewHome )
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Vector migration only currently supported for individual (not cohort) model." );
    }

    const suids::suid& VectorCohort::GetMigrationDestination()
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Vector migration only currently supported for individual (not cohort) model." );
    }

    MigrationType::Enum VectorCohort::GetMigrationType() const
    {
        throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Vector migration only currently supported for individual (not cohort) model." );
    }

    VectorCohort::~VectorCohort()
    {
    }

    int32_t VectorCohort::GetPopulation() const
    {
        return population;
    }

    void VectorCohort::SetPopulation(
        int32_t new_pop
    )
    {
        population = new_pop;
    }

    double 
    VectorCohort::GetProgress() const
    {
        return progress;
    }

    void
    VectorCohort::ClearProgress()
    {
        progress = 0;
    }

    void
    VectorCohort::IncreaseProgress( double delta )
    {
        progress += delta;
    }

    VectorMatingStructure& 
    VectorCohort::GetVectorGenetics()
    {
        return vector_genetics;
    }

    void
    VectorCohort::SetVectorGenetics( const VectorMatingStructure& new_value )
    {
        vector_genetics = new_value;
    }

    IMigrate* VectorCohort::GetIMigrate()
    {
        return static_cast<IMigrate*>(this);
    }


    /*VectorCohort::~VectorCohort()
    {
        LOG_VALID( "VectorCohort destructor.\n" );
    }*/

    float
    VectorCohort::GetMortality( uint32_t addition ) const
    {
        return addition;
        //throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "VectorCohort::GetMortality" );
    }

    REGISTER_SERIALIZABLE(VectorCohort);

    void VectorCohort::serialize(IArchive& ar, VectorCohort* obj)
    {
        VectorCohort& cohort = *obj;
        ar.labelElement("vector_genetics");
        VectorMatingStructure::serialize(ar, cohort.vector_genetics);
        ar.labelElement("progress") & cohort.progress;
        ar.labelElement("population") & cohort.population;
    }
}
