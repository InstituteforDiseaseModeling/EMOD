/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "VectorCohortAging.h"
#include "Common.h"
#include "BoostLibWrapper.h"

namespace Kernel
{
    class IVectorCohortIndividual : public ISupports
    {
    public:
        virtual VectorStateEnum::Enum & GetState() = 0;
        virtual void SetState( const VectorStateEnum::Enum & ) = 0;
        virtual float GetAdditionalMortality() const = 0;
        virtual float GetOvipositionTimer() = 0;
        virtual int GetParity() = 0;
        virtual int GetNewEggs() = 0;
        virtual const std::string &GetSpecies() = 0;
        virtual const suids::suid & GetMigrationDestination() = 0;
        virtual const StrainIdentity* GetStrainIdentity() const = 0;
 
        virtual void IncrementParity() = 0 ;
        virtual void ReduceOvipositionTimer( float delta ) = 0;
        virtual void SetAdditionalMortality( float new_mortality ) = 0;
        virtual void SetNewEggs( int new_eggs ) = 0;
        virtual void SetOvipositionTimer( float new_opt ) = 0;
        virtual void AcquireNewInfection( StrainIdentity *infstrain = NULL, int incubation_period_override = -1 ) = 0;
        virtual bool IsProgressedOrEmpty() const = 0;
        virtual float GetMortality( uint32_t addition ) const = 0;

        // VectorCohortAging base class wrappers
        virtual float GetAge() const = 0;
        virtual void IncreaseAge( float dt ) = 0;
    };

    class VectorCohortIndividual : public IVectorCohortIndividual, public VectorCohortAging
 
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorCohortIndividual *CreateCohort(VectorStateEnum::Enum state = VectorStateEnum::STATE_ADULT, float age = 0.0f, float progress = 0.0f, int32_t initial_population = 100, VectorMatingStructure = VectorMatingStructure(), std::string vector_species_name = "gambiae");
        virtual ~VectorCohortIndividual();

        // should we bother with an interface for this common function to individual human? IInfectable?
        virtual void AcquireNewInfection( StrainIdentity *infstrain = NULL, int incubation_period_override = -1 );
        virtual const StrainIdentity* GetStrainIdentity() const;

        // IMigrate interfaces
        virtual void ImmigrateTo(Node* destination_node);
        virtual void SetMigrationDestination(suids::suid destination);
        virtual const suids::suid& GetMigrationDestination();

        virtual VectorStateEnum::Enum & GetState();
        virtual void SetState( const VectorStateEnum::Enum & );
        virtual float GetAdditionalMortality() const;
        virtual float GetOvipositionTimer();
        virtual int GetParity();
        virtual int GetNewEggs();
        virtual const std::string &GetSpecies();
        virtual void IncrementParity();
        virtual void ReduceOvipositionTimer( float delta );
        virtual void SetAdditionalMortality( float new_mortality );
        virtual void SetNewEggs( int new_eggs );
        virtual void SetOvipositionTimer( float new_opt );
        virtual bool IsProgressedOrEmpty() const;

        virtual float GetMortality( uint32_t addition ) const;

        // base class wrappers
        virtual float GetAge() const;
        virtual void IncreaseAge( float dt );

    protected:
        VectorCohortIndividual();
        VectorCohortIndividual(VectorStateEnum::Enum state, float age, float progress, int32_t initial_population, VectorMatingStructure _vector_genetics, std::string vector_species_name);
        void Initialize();

        VectorStateEnum::Enum state;
        float additional_mortality;
        float oviposition_timer;
        int parity;
        int neweggs;
        suids::suid migration_destination;
        std::string species;

        StrainIdentity* m_strain;

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, VectorCohortIndividual& cohort, const unsigned int  file_version );
#endif
    };
}
