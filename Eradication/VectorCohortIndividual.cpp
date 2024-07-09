
#include "stdafx.h"

#include "VectorCohortIndividual.h"
#include "Exceptions.h"
#include "Log.h"
#include "VectorParameters.h"

SETUP_LOGGING( "VectorCohortIndividual" )

namespace Kernel
{

    // QI stuff
    BEGIN_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAbstract )
        HANDLE_INTERFACE( IVectorCohortIndividual )
    END_QUERY_INTERFACE_DERIVED(VectorCohortIndividual, VectorCohortAbstract)

    VectorCohortIndividual::VectorCohortIndividual() 
    : VectorCohortAbstract()
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f)
    , parity(0)
    , num_gestating(0)
    , m_pStrain( nullptr )
    {
    }

    VectorCohortIndividual::VectorCohortIndividual( uint32_t vectorID,
                                                    VectorStateEnum::Enum _state, 
                                                    float _age,
                                                    float _progress,
                                                    float microsporidiaDuration,
                                                    uint32_t _initial_population,
                                                    const VectorGenome& rGenome,
                                                    int speciesIndex )
    : VectorCohortAbstract( vectorID,
                            _state,
                            _age,
                            _progress,
                            microsporidiaDuration,
                            _initial_population,
                            rGenome,
                            speciesIndex )
    , additional_mortality(0.0f)
    , oviposition_timer(-0.1f) // newly-mated mosquitoes feed on first cycle
    , parity(0)
    , num_gestating(0)
    , m_pStrain( nullptr )
    {
    }

    VectorCohortIndividual::~VectorCohortIndividual()
    {
        delete m_pStrain;
    }

    void VectorCohortIndividual::Initialize()
    {
        VectorCohortAbstract::Initialize();
    }

    VectorCohortIndividual *VectorCohortIndividual::CreateCohort( uint32_t vectorID,
                                                                  VectorStateEnum::Enum _state,
                                                                  float _age,
                                                                  float _progress,
                                                                  float microsporidiaDuration,
                                                                  uint32_t _initial_population,
                                                                  const VectorGenome& rGenome,
                                                                  int speciesIndex )
    {
        VectorCohortIndividual *newqueue;
        
        if (!_supply || (_supply->size() == 0))
        {
            newqueue = _new_ VectorCohortIndividual( vectorID,
                                                     _state,
                                                     _age,
                                                     _progress,
                                                     microsporidiaDuration,
                                                     _initial_population,
                                                     rGenome,
                                                     speciesIndex );
        }
        else
        {
            VectorCohortIndividual* ptr = _supply->back();
            _supply->pop_back();
            newqueue = new(ptr) VectorCohortIndividual( vectorID,
                                                        _state,
                                                        _age,
                                                        _progress,
                                                        microsporidiaDuration,
                                                        _initial_population,
                                                        rGenome,
                                                        speciesIndex );
        }
        newqueue->Initialize();

        return newqueue;
    }

    void VectorCohortIndividual::AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override )
    {
        if ( state != VectorStateEnum::STATE_ADULT )
        {
            // A mosquito can be exposed to multiple contagion populations (one for each antigen each time step) but
            // we don't model the vector having multiple infections.
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Error trying to infect an already infected mosquito." );
        }
        else
        {
            progress = 0;
            state    = VectorStateEnum::STATE_INFECTED;
            m_pStrain = infstrain->Clone();
        }
    }

    const IStrainIdentity* VectorCohortIndividual::GetStrainIdentity() const
    {
        return m_pStrain;
    }

    bool VectorCohortIndividual::HasStrain( const IStrainIdentity& rStrain ) const
    {
        if( m_pStrain == nullptr )
        {
            return false;
        }
        else
        {
            return (m_pStrain->GetGeneticID() == rStrain.GetGeneticID());
        }
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
        // Remember that this is a virtual method like other virtual methods.
        // A subclass' destructor method will be called.
        vci->~VectorCohortIndividual();
        _supply->push_back(vci);
    }

    void
    VectorCohortIndividual::IncrementParity()
    {
        parity++;
    }

    void
    VectorCohortIndividual::SetOvipositionTimer(
        int new_opt
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

    int VectorCohortIndividual::GetOvipositionTimer() const
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

    IVectorCohort* VectorCohortIndividual::SplitPercent( RANDOMBASE* pRNG, uint32_t newVectorID, float percentLeaving )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Individuals should not Split" );
    }

    IVectorCohort* VectorCohortIndividual::SplitNumber( RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Individuals should not Split" );
    }

    uint32_t VectorCohortIndividual::GetNumLookingToFeed() const
    {
        return (population - num_gestating);
    }

    void VectorCohortIndividual::AddNewGestating( uint32_t daysToGestate, uint32_t newGestating )
    {
        num_gestating = newGestating;
        if( num_gestating > 0 )
        {
            oviposition_timer = int(daysToGestate);
        }
        else
        {
            oviposition_timer = -1; // ensure timer is expired
        }
    }

    uint32_t VectorCohortIndividual::GetNumGestating() const
    {
        return num_gestating;
    }

    uint32_t VectorCohortIndividual::RemoveNumDoneGestating()
    {
        uint32_t num = num_gestating;
        num_gestating = 0;
        return num;
    }

    uint32_t VectorCohortIndividual::AdjustGestatingForDeath( RANDOMBASE* pRNG, float percentDied, bool killGestatingOnly )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be called for individuals" );
    }

    const std::vector<uint32_t>& VectorCohortIndividual::GetGestatingQueue() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be called for individuals" );
    }

    void VectorCohortIndividual::ReportOnGestatingQueue( std::vector<uint32_t>& rNumGestatingQueue ) const
    {
        if( num_gestating > 0 )
        {
            release_assert( oviposition_timer >= 0 );
            uint32_t day = oviposition_timer;
            if( day < 1 )
            {
                day = 1;
            }

            uint32_t index = day - 1;
            while( index >= rNumGestatingQueue.size() )
            {
                rNumGestatingQueue.push_back( 0 );
            }
            rNumGestatingQueue[ index ] += num_gestating;
        }
    }

    VectorHabitatType::Enum VectorCohortIndividual::GetHabitatType()
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "should not be called on individual vectors" );
    }

    IVectorHabitat* VectorCohortIndividual::GetHabitat()
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "should not be called on individual vectors" );
    }

    void VectorCohortIndividual::SetHabitat( IVectorHabitat* new_habitat )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "should not be called on individual vectors" );
    }

    REGISTER_SERIALIZABLE(VectorCohortIndividual);

    void VectorCohortIndividual::serialize(IArchive& ar, VectorCohortIndividual* obj)
    {
        VectorCohortAbstract::serialize(ar, obj);
        VectorCohortIndividual& cohort = *obj;
        ar.labelElement("additional_mortality" ) & cohort.additional_mortality;
        ar.labelElement("oviposition_timer"    ) & cohort.oviposition_timer;
        ar.labelElement("parity"               ) & cohort.parity;
        ar.labelElement("num_gestating"        ) & cohort.num_gestating;
        ar.labelElement("m_pStrain"            ) & cohort.m_pStrain;
    }
}
