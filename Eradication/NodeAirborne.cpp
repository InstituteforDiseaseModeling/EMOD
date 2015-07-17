/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "NodeAirborne.h"

#include "IndividualAirborne.h"

static const char * _module = "NodeAirborne";

namespace Kernel
{
    NodeAirborne::NodeAirborne(ISimulationContext *_parent_sim, suids::suid node_suid)
    : Node(_parent_sim, node_suid)
    {
    }

    NodeAirborne::NodeAirborne() : Node()
    {
    }

    NodeAirborne::~NodeAirborne(void)
    {
    }

    NodeAirborne *NodeAirborne::CreateNode(ISimulationContext *_parent_sim, suids::suid node_suid)
    {
        NodeAirborne *newnode = _new_ NodeAirborne(_parent_sim, node_suid);
        newnode->Initialize();

        return newnode;
    }

    IndividualHuman *NodeAirborne::createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float above_poverty)
    {
        return IndividualHumanAirborne::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender, above_poverty);
    }

    float NodeAirborne::getClimateInfectivityCorrection() const
    {
        // Airborne infectivity depends on relative humidity.
        // TODO: make more configurable to accommodate different modalities:
        //       - primarily indoor transmission with controlled temperatures
        //       - relative importance of settling, ventilation, inactivation
        //       - temperature/humidity effects on contagion viability

        if ( localWeather == NULL )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "localWeather", "Climate");
            //throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "climate_structure", "CLIMATE_OFF", "infectivity_scaling", "FUNCTION_OF_CLIMATE");
        }

        float humidity = localWeather->humidity();

        // The following is a sigmoidal form that drops from 1 to 0.5 mostly in the range from 30% to 70% relative humidity
        float correction = 0.75 - 0.2 * atan( 6 * humidity - 3 ); // linux build breaker?
        LOG_DEBUG_F( "Infectivity scale factor = %f at relative humidity = %f.\n", correction, humidity );

        return correction;
    }
}

#if USE_BOOST_SERIALIZATION
#include "IndividualAirborne.h"
BOOST_CLASS_EXPORT(Kernel::NodeAirborne)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, NodeAirborne& node, const unsigned int file_version)
    {
        // Register derived types
        //ar.template register_type<IndividualHumanAirborne>();

        // Serialize base class
        ar &boost::serialization::base_object<Node>(node);    
    }
}
#endif

#endif // ENABLE_TB
