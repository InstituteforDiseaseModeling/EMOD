
#pragma once

#include "VectorCohort.h"
#include "StrainIdentity.h"

namespace Kernel
{
    struct IVectorCohortIndividual : ISupports
    {
        virtual int GetOvipositionTimer() const = 0;
        virtual int GetParity() = 0;
        virtual const IStrainIdentity* GetStrainIdentity() const = 0;
        virtual bool HasStrain( const IStrainIdentity& rStrain ) const = 0;
 
        virtual void IncrementParity() = 0 ;
        virtual void SetOvipositionTimer( int new_opt ) = 0;
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override = -1) = 0;
        virtual bool IsProgressedOrEmpty() const = 0;
    };

    class VectorCohortIndividual : public VectorCohortAbstract, public IVectorCohortIndividual
    {
    public:
        DECLARE_QUERY_INTERFACE()
        virtual int32_t AddRef() override { return 1; }
        virtual int32_t Release() override { return 1; }

    public:
        static VectorCohortIndividual *CreateCohort( uint32_t vectorID,
                                                     VectorStateEnum::Enum state, 
                                                     float age, 
                                                     float progress, 
                                                     float microsporidiaDuration,
                                                     uint32_t initial_population,
                                                     const VectorGenome& rGenome,
                                                     int speciesIndex );
        static void reclaim( IVectorCohortIndividual* ivci );

        virtual ~VectorCohortIndividual();

        virtual void Merge( IVectorCohort* pCohortToAdd ) override;
        virtual IVectorCohort* SplitPercent( RANDOMBASE* pRNG, uint32_t newVectorID, float percentLeaving ) override;
        virtual IVectorCohort* SplitNumber(  RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving  ) override;
        virtual uint32_t GetNumLookingToFeed() const override;
        virtual void AddNewGestating( uint32_t daysToGestate, uint32_t newGestating ) override;
        virtual uint32_t GetNumGestating() const override;
        virtual uint32_t RemoveNumDoneGestating() override;
        virtual uint32_t AdjustGestatingForDeath( RANDOMBASE* pRNG, float percentDied, bool killGestatingOnly ) override;
        virtual const std::vector<uint32_t>& GetGestatingQueue() const override;
        virtual void ReportOnGestatingQueue( std::vector<uint32_t>& rNumGestatingQueue ) const override;

        virtual VectorHabitatType::Enum GetHabitatType() override;
        virtual IVectorHabitat* GetHabitat() override;
        virtual void SetHabitat( IVectorHabitat* ) override;

        // should we bother with an interface for this common function to individual human? IInfectable?
        virtual void AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override = -1);
        virtual const IStrainIdentity* GetStrainIdentity() const override;
        virtual bool HasStrain( const IStrainIdentity& rStrain ) const override;

        virtual int GetOvipositionTimer() const override;
        virtual int GetParity() override;
        virtual void IncrementParity() override;
        virtual void SetOvipositionTimer( int new_opt ) override;
        virtual bool IsProgressedOrEmpty() const override;

        static void DeleteSupply()
        {
            if( _supply != nullptr )
            {
                for( auto p_vci : *_supply )
                {
                    delete p_vci;
                }
                (* _supply).clear();
                delete _supply;
                _supply = nullptr;
            }
        }

    protected:
        VectorCohortIndividual();
        VectorCohortIndividual( uint32_t vectorID,
                                VectorStateEnum::Enum state,
                                float age,
                                float progress,
                                float microsporidiaDuration,
                                uint32_t initial_population,
                                const VectorGenome& rGenome,
                                int speciesIndex );

        virtual void Initialize() override;

        float additional_mortality;
        int oviposition_timer;
        int parity;
        uint32_t num_gestating;
        IStrainIdentity* m_pStrain;

        static std::vector<VectorCohortIndividual*>* _supply;

        DECLARE_SERIALIZABLE(VectorCohortIndividual);

    private:
        // disable copy constructor
        VectorCohortIndividual( const VectorCohortIndividual& rThat ) = delete;
    };
}
