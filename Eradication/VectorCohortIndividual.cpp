/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "VectorCohortIndividual.h"
#include "Exceptions.h"
#include "Log.h"
#include "VectorParameters.h"

SETUP_LOGGING( "VectorCohortIndividual" )

namespace Kernel
{
    static uint64_t VCI_COUNTER = 1 ; //TODO - need to make multi-core

    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAbstract )
        HANDLE_INTERFACE( IVectorCohortIndividual )
    END_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAbstract)

    VectorCohortIndividual::VectorCohortIndividual() 
    : VectorCohortAbstract()
    , m_ID( VCI_COUNTER++ )
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f)
    , parity(0)
    , neweggs(0)
    , m_strain(0, 0)
    {
    }

    VectorCohortIndividual::VectorCohortIndividual( VectorStateEnum::Enum _state, 
                                                    float _age,
                                                    float _progress,
                                                    uint32_t _initial_population,
                                                    const VectorMatingStructure& _vector_genetics,
                                                    const std::string* _vector_species_name )
    : VectorCohortAbstract( _state, _age, _progress, _initial_population, _vector_genetics, _vector_species_name )
    , m_ID( VCI_COUNTER++ )
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f) // newly-mated mosquitoes feed on first cycle
    , parity(0)
    , neweggs(0)
    , m_strain(0, 0)
    {
    }

    void VectorCohortIndividual::Initialize()
    {
        VectorCohortAbstract::Initialize();
    }

    VectorCohortIndividual *VectorCohortIndividual::CreateCohort( VectorStateEnum::Enum _state,
                                                                  float _age,
                                                                  float _progress,
                                                                  uint32_t _initial_population,
                                                                  const VectorMatingStructure& _vector_genetics,
                                                                  const std::string* _vector_species_name )
    {
        VectorCohortIndividual *newqueue;
        
        if (!_supply || (_supply->size() == 0))
        {
            newqueue = _new_ VectorCohortIndividual( _state, _age, _progress, _initial_population, _vector_genetics, _vector_species_name);
        }
        else
        {
            VectorCohortIndividual* ptr = _supply->back();
            _supply->pop_back();
            newqueue = new(ptr) VectorCohortIndividual( _state, _age, _progress, _initial_population, _vector_genetics, _vector_species_name);
        }
        newqueue->Initialize();

        return newqueue;
    }

    void VectorCohortIndividual::AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override )
    {
        if ( state != VectorStateEnum::STATE_ADULT )
        {
            // removing this as a mosquito can be exposed to multiple contagion populations (one for each antigen each time step) throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Error trying to infect an already infected mosquito." );
        }
        else
        {
            progress = 0;
            state    = VectorStateEnum::STATE_INFECTED;
            m_strain = StrainIdentity( infstrain );
        }
    }

    const IStrainIdentity& VectorCohortIndividual::GetStrainIdentity() const
    {
        return m_strain;
    }

    VectorCohortIndividual::~VectorCohortIndividual()
    {
    }

    std::vector<VectorCohortIndividual*>* VectorCohortIndividual::_supply = nullptr;

    void VectorCohortIndividual::reclaim(IVectorCohortIndividual* ivci)
    {
        if (!_supply)
        {
            _supply = new std::vector<VectorCohortIndividual*>();
            _supply->reserve(65535);
        }
        VectorCohortIndividual* vci = static_cast<VectorCohortIndividual*>(ivci);
        vci->~VectorCohortIndividual();
        _supply->push_back(vci);
    }

    void
    VectorCohortIndividual::IncrementParity()
    {
        parity++;
    }

    void
    VectorCohortIndividual::ReduceOvipositionTimer(
        float delta
    )
    {
        oviposition_timer -= delta;
    }

    void
    VectorCohortIndividual::SetOvipositionTimer(
        float new_opt
    )
    {
        oviposition_timer = new_opt;
    }

    bool
    VectorCohortIndividual::IsProgressedOrEmpty()
    const
    {
        if( progress >= 1 || population <= 0 )
        {
            return true;
        }
        return false;
    }

    float VectorCohortIndividual::GetOvipositionTimer()
    {
        return oviposition_timer;
    }

    int VectorCohortIndividual::GetParity()
    {
        return parity;
    }

    void VectorCohortIndividual::Merge( IVectorCohort* pCohortToAdd )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Individuals should not merge" );
    }

    IVectorCohort* VectorCohortIndividual::Split( uint32_t numLeaving )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Individuals should not Split" );
    }

    void VectorCohortIndividual::AddNewEggs( uint32_t daysToGestate, uint32_t new_eggs )
    {
        neweggs = new_eggs;

        // Make sure that floating-point precision doesn't delay things by an extra day!
        oviposition_timer = float(daysToGestate) - 0.01f;
    }

    uint32_t VectorCohortIndividual::GetGestatedEggs()
    {
        return neweggs;
    }

    void VectorCohortIndividual::AdjustEggsForDeath( uint32_t numDied )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be called for individuals" );
    }

    const std::vector<uint32_t>& VectorCohortIndividual::GetNewEggs() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be called for individuals" );
    }

    REGISTER_SERIALIZABLE(VectorCohortIndividual);

    void VectorCohortIndividual::serialize(IArchive& ar, VectorCohortIndividual* obj)
    {
        VectorCohortAbstract::serialize(ar, obj);
        VectorCohortIndividual& cohort = *obj;
        //ar.labelElement("m_ID"                 ) & cohort.m_ID;
        ar.labelElement("additional_mortality" ) & cohort.additional_mortality;
        ar.labelElement("oviposition_timer"    ) & cohort.oviposition_timer;
        ar.labelElement("parity"               ) & cohort.parity;
        ar.labelElement("neweggs"              ) & cohort.neweggs;

        /* TODO - this 'has_strain' stuff is left over from when m_strain could be a nullptr. */
        bool has_strain = true;
        ar.labelElement("__has_strain__");
        ar & has_strain;
        // clorton TODO - perhaps the cohort should always have a non-null m_strain, just use a dummy StrainIdentity when there's no infection.
        if (has_strain)
        {
            ar.labelElement("m_strain");
            StrainIdentity::serialize(ar, cohort.m_strain);
        }
    }
}
