/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorContexts.h"

namespace Kernel
{
    // Contain and handle the various transition probabilities of the vector population life cycle
    class VectorProbabilities
    {
    public:
        static VectorProbabilities* CreateVectorProbabilities();
        virtual ~VectorProbabilities();

        void ResetProbabilities();
        void AccumulateIndividualProbabilities(IVectorInterventionsEffects* ivie, float weight);
        void NormalizeIndividualProbabilities();
        void SetNodeProbabilities(INodeVectorInterventionEffects* invie, float dt);
        void FinalizeTransitionProbabilites(float anthropophily, float indoor_feeding);

        // vector-feeding and MC weighted population
        float effective_host_population;

        // first set is outcomes until feeding
        float outdoorareakilling;
        float outdoorareakilling_male;
        float diebeforeattempttohumanfeed;
        float diewithoutattemptingfeed;
        float survivewithoutsuccessfulfeed;
        float successfulfeed_animal;
        float successfulfeed_AD;
        float indoorattempttohumanfeed;
        float outdoorattempttohumanfeed;

        // outdoor feeding probabilities are simple 
        float outdoor_returningmortality;

        // indoor feeding outcomes
        float indoor_diebeforefeeding;
        float indoor_hostnotavailable;
        float indoor_dieduringfeeding;
        float indoor_diepostfeeding;
        float indoor_successfulfeed_human;
        float indoor_successfulfeed_AD;

        // outdoor feeding outcomes
        float outdoor_diebeforefeeding;
        float outdoor_hostnotavailable;
        float outdoor_dieduringfeeding;
        float outdoor_diepostfeeding;
        float outdoor_successfulfeed_human;

        // other outcomes
        float sugarTrapKilling;
        float individualRepellentBlock;

        // intermediate values that are combined to arrive at "before-feed" summary probabilities
        float attraction_ADOV;
        float attraction_ADIV;
        float kill_livestockfeed;
        float kill_PFV;
        float spatial_repellent;
        float nooutdoorhumanfound;
        float outdoorRestKilling;

        static void serialize(IArchive&, VectorProbabilities*&);

    protected:
        VectorProbabilities();
    };
}
