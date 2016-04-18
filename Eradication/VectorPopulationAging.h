/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorPopulation.h"

#include "BoostLibWrapper.h"

#include "Common.h"

// References for senescence:
// Clements, A. N. and G. D. Paterson (1981). "The analysis of mortality and survival rates in wild populations of mosquitoes." Journal of Applied Ecology 18: 373-399.
// Killeen, G., F. McKenzie, et al. (2000). "A simplified model for predicting malaria entomologic inoculation rates based on entomologic and parasitologic parameters relevant to control." Am J Trop Med Hyg 62(5): 535-544.

namespace Kernel
{
    class VectorPopulationAging : public VectorPopulation
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static VectorPopulationAging *CreatePopulation(INodeContext *context, std::string species_name = "gambiae", int32_t intial_adults = DEFAULT_VECTOR_POPULATION_SIZE, int32_t initial_infectious = 0);
        VectorPopulationAging();
        virtual ~VectorPopulationAging();

        virtual void Update_Infectious_Queue( float dt );
        virtual void Update_Infected_Queue  ( float dt );
        virtual void Update_Adult_Queue     ( float dt );
        virtual void Update_Immature_Queue  ( float dt );
        virtual void Update_Male_Queue      ( float dt );

        virtual void AddVectors(VectorMatingStructure _vector_genetics, unsigned long int releasedNumber);

    protected:
        virtual void InitializeVectorQueues(unsigned int adults, unsigned int _infectious);
    };
}
