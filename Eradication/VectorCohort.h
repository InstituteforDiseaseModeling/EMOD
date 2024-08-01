
#pragma once

#include <list>
#include "suids.hpp"
#include "IMigrate.h"
#include "Vector.h"
#include "ISerializable.h"
#include "IVectorCohort.h"
#include "VectorGenome.h"

namespace Kernel
{
    struct INodeContext;

    class VectorCohortAbstract : public IVectorCohort, public IMigrate
    {
    public:
        virtual int32_t AddRef() override { return 1; }
        virtual int32_t Release() override { return 1; }
        DECLARE_QUERY_INTERFACE()

    public:
        static uint32_t I_MAX_AGE;

        virtual ~VectorCohortAbstract();

        // IMigrate interfaces
        virtual void ImmigrateTo(INodeContext* destination_node) override;
        virtual void SetMigrating( suids::suid destination, 
                                   MigrationType::Enum type, 
                                   float timeUntilTrip, 
                                   float timeAtDestination,
                                   bool isDestinationNewHome ) override;
        virtual const suids::suid & GetMigrationDestination() override;
        virtual MigrationType::Enum GetMigrationType() const  override;

        virtual uint32_t GetID() const override { return m_ID; }
        virtual int GetSpeciesIndex() const override;
        virtual const VectorGenome& GetGenome() const override;
        virtual void SetMateGenome( const VectorGenome& rGenomeMate ) override;
        virtual const VectorGenome& GetMateGenome() const override;
        virtual bool HasMated() const override;
        virtual VectorStateEnum::Enum GetState() const override;
        virtual void SetState( VectorStateEnum::Enum _state ) override;
        virtual const std::string &GetSpecies() override;
        virtual VectorWolbachia::Enum GetWolbachia() const override;
        virtual uint32_t GetPopulation() const override; // used by VPI
        virtual void SetPopulation( uint32_t new_pop ) override; // used by VPI (1x besides ClearPop)
        virtual float GetProgress() const override; // NOT used by VPI
        virtual void ClearProgress() override; // NOT used by VPI, implicit
        virtual IMigrate* GetIMigrate() override;
        virtual float GetAge() const override;
        virtual void SetAge( float ageDays ) override;
        virtual void IncreaseAge( float dt ) override;
        virtual void Update( RANDOMBASE* pRNG,
                             float dt,
                             const VectorTraitModifiers& rTraitModifiers,
                             float progressThisTimestep,
                             bool hadMicrosporidiaPreviously ) override;
        virtual bool HasWolbachia() const override;
        virtual bool HasMicrosporidia() const override;
        virtual void InfectWithMicrosporidia( int strainIndex ) override;
        virtual float GetDurationOfMicrosporidia() const override;

    protected:
        VectorCohortAbstract();
        VectorCohortAbstract( const VectorCohortAbstract& rThat );
        VectorCohortAbstract( uint32_t vectorID,
                              VectorStateEnum::Enum state,
                              float age,
                              float progress,
                              float microsporidiaDuration,
                              uint32_t population, 
                              const VectorGenome& rGenome,
                              int speciesIndex );
        virtual void Initialize();

        void IncreaseProgress( float delta );
        void SetProgress( float newProgress );

        uint32_t m_ID;
        int species_index;
        VectorGenome genome_self;
        VectorGenome genome_mate;
        VectorStateEnum::Enum state;
        float progress;
        uint32_t population;
        float age;
        MigrationType::Enum migration_type;
        suids::suid migration_destination;
        float microsporidia_infection_duration;

        static void serialize( IArchive& ar, VectorCohortAbstract* obj );
    };

    class VectorCohort : public VectorCohortAbstract
    {
    public:
        DECLARE_QUERY_INTERFACE()

        static VectorCohort *CreateCohort( uint32_t vectorID,
                                           VectorStateEnum::Enum state,
                                           float age,
                                           float progress,
                                           float microsporidiaDuration,
                                           uint32_t population,
                                           const VectorGenome& rGenome,
                                           int speciesIndex );
        static VectorCohort *CreateCohort( IVectorHabitat* _habitat,
                                           uint32_t vectorID,
                                           VectorStateEnum::Enum state,
                                           float progress,
                                           float microsporidiaDuration,
                                           uint32_t initial_population,
                                           const VectorGenome& rGenome,
                                           int speciesIndex );
        virtual ~VectorCohort();

        virtual void SetPopulation( uint32_t newPop ) override;
        virtual void Merge( IVectorCohort* pCohortToAdd ) override;
        virtual IVectorCohort* SplitPercent( RANDOMBASE* pRNG, uint32_t newVectorID, float percentLeaving ) override;
        virtual IVectorCohort* SplitNumber(  RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving  ) override;
        virtual uint32_t GetNumLookingToFeed() const override;
        virtual void AddNewGestating( uint32_t daysToGestate, uint32_t newFed ) override;
        virtual uint32_t GetNumGestating() const override;
        virtual uint32_t RemoveNumDoneGestating() override;
        virtual uint32_t AdjustGestatingForDeath( RANDOMBASE* pRNG, float percentDied, bool killGestatingOnly ) override;
        virtual const std::vector<uint32_t>& GetGestatingQueue() const override;
        virtual void ReportOnGestatingQueue( std::vector<uint32_t>& rNumGestatingQueue ) const override;

        virtual VectorHabitatType::Enum GetHabitatType() override;
        virtual IVectorHabitat* GetHabitat() override;
        virtual void SetHabitat( IVectorHabitat* ) override;


    protected:
        VectorCohort();
        VectorCohort( IVectorHabitat* _habitat,
                      uint32_t vectorID,
                      VectorStateEnum::Enum state,
                      float age,
                      float progress,
                      float microsporidiaDuration,
                      uint32_t population,
                      const VectorGenome& rGenome,
                      int speciesIndex );

        std::vector<uint32_t> gestating_queue;
        uint32_t total_gestating;
        VectorHabitatType::Enum habitat_type;
        IVectorHabitat* habitat;

        DECLARE_SERIALIZABLE(VectorCohort);

    private:
        // keep private so it can only be used in Split()
        VectorCohort( const VectorCohort& rThat );
    };


    class VectorCohortMale : public VectorCohortAbstract
    {
    public:
        DECLARE_QUERY_INTERFACE()

        static VectorCohortMale* CreateCohort(uint32_t vectorID,
            VectorStateEnum::Enum state,
            float age,
            float progress,
            float microsporidiaDuration,
            uint32_t population,
            const VectorGenome& rGenome,
            int speciesIndex);

        virtual ~VectorCohortMale();

        
        virtual void Merge(IVectorCohort* pCohortToAdd) override;
        virtual VectorCohortMale* SplitPercent(RANDOMBASE* pRNG, uint32_t newVectorID, float percentLeaving) override;
        virtual VectorCohortMale* SplitNumber(RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving) override;
        virtual uint32_t GetNumLookingToFeed() const override 
        { 
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }
        virtual void AddNewGestating(uint32_t daysToGestate, uint32_t newFed) override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }
        virtual uint32_t GetNumGestating() const override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }
        virtual uint32_t RemoveNumDoneGestating() override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }
        virtual uint32_t AdjustGestatingForDeath(RANDOMBASE* pRNG, float percentDied, bool killGestatingOnly) override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }
        virtual const std::vector<uint32_t>& GetGestatingQueue() const override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }
        virtual void ReportOnGestatingQueue(std::vector<uint32_t>& rNumGestatingQueue)  const override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }

        virtual VectorHabitatType::Enum GetHabitatType() override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }
        virtual IVectorHabitat* GetHabitat() override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }
        virtual void SetHabitat(IVectorHabitat*) override
        {
            throw Kernel::IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "The method is not part of VectorCohortMale.");
        }

        uint32_t GetUnmatedCount() const;
        void SetUnmatedCount(uint32_t unmated_count);
        uint32_t GetUnmatedCountCDF() const;
        void SetUnmatedCountCDF(uint32_t unmated_count_cdf);
        VectorCohortMale* SplitHelper(RANDOMBASE* pRNG, uint32_t newVectorID, uint32_t numLeaving, float percentLeaving);

    protected:
        VectorCohortMale();
        VectorCohortMale(IVectorHabitat* _habitat,
            uint32_t vectorID,
            VectorStateEnum::Enum state,
            float age,
            float progress,
            float microsporidiaDuration,
            uint32_t population,
            const VectorGenome& rGenome,
            int speciesIndex);
        uint32_t unmated_count;
        uint32_t unmated_count_cdf;

        DECLARE_SERIALIZABLE(VectorCohortMale);

    private:
        // keep private so it can only be used in Split()
        VectorCohortMale(const VectorCohortMale& rThat);

    };
}
