/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "Node.h"
#include "IndividualAirborne.h" // for serialization only

namespace Kernel
{
    class NodeAirborne : public Node
    {
    public:
        virtual ~NodeAirborne(void);
        static NodeAirborne *CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid);

    protected:
        NodeAirborne();
        NodeAirborne(ISimulationContext *_parent_sim, suids::suid node_suid);

        // Factory methods
        virtual IndividualHuman *createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float above_poverty);

        // Effect of climate on infectivity in airborne disease
        virtual float getClimateInfectivityCorrection()  const;

    private:
#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, NodeAirborne& node, const unsigned int file_version);
#endif
    };
}
