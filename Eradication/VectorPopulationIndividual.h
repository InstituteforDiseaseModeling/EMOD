/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "VectorPopulation.h"

#include "Common.h"

#include "BoostLibWrapper.h"

namespace Kernel
{
    class IVectorCohortIndividual;

    class VectorPopulationIndividual : public VectorPopulation
    {

        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        VectorPopulationIndividual() : m_mosquito_weight(0), m_average_oviposition_killing(0) {};
        static VectorPopulationIndividual *CreatePopulation(INodeContext *context, std::string species_name = "gambiae", int32_t _adult = DEFAULT_VECTOR_POPULATION_SIZE, int32_t _infectious = 0, uint32_t mosquito_weight = 1);
        virtual ~VectorPopulationIndividual();

        virtual void Update_Infectious_Queue( float dt );
        virtual void Update_Infected_Queue  ( float dt );
        virtual void Update_Adult_Queue     ( float dt );
        virtual void Update_Immature_Queue  ( float dt );
        virtual void Update_Male_Queue      ( float dt );

        // function to support adding in new vectors (Wolbachia types, etc...)
        virtual void AddVectors(VectorMatingStructure _vector_genetics, unsigned long int releasedNumber);

        // IInfectable
        virtual void Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route );

        virtual unsigned long int Vector_Migration(float = 0.0f, VectorCohortList_t * = NULL);

    protected:
        uint32_t m_mosquito_weight;
        float m_average_oviposition_killing;

        // Temporary containers to store exposed vectors before exposing them to infectious strains
        // These could be moved back to the base class, VectorPopulation, if we figure out a sensible way 
        // to deal with minimal strain tracking in the cohort model
        VectorCohortList_t IndoorExposedQueues;
        VectorCohortList_t OutdoorExposedQueues;

        VectorPopulationIndividual(uint32_t mosquito_weight);
        virtual void InitializeVectorQueues(unsigned int adults, unsigned int _infectious);
        virtual uint32_t ProcessFeedingCycle(float dt, VectorCohort *queue, VectorStateEnum::Enum state);
        void ExposeCohortList( const IContagionPopulation* cp, VectorCohortList_t& list, float success_prob, float infection_prob );
        void ResetOvipositionTimer( IVectorCohortIndividual* mosquito );
        IVectorCohortIndividual * current_vci; // added this since we have it, then call a function, and function re-qi's for it, which is unnecessary.

    private:

#if USE_BOOST_SERIALIZATION
        // Serialization
        template<class Archive>
        friend void serialize(Archive & ar, VectorPopulationIndividual &vpi, const unsigned int  file_version );
#endif
    };
}
