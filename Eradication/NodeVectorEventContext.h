/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "ISupports.h"
#include "Configuration.h"
#include "suids.hpp"
#include "RANDOM.h"
#include "NodeEventContext.h"
#include "NodeEventContextHost.h"
#include "VectorEnums.h"
#include "VectorContexts.h"
#include "VectorMatingStructure.h"
#include "Types.h"

namespace Kernel
{
    class Simulation;

    class INodeVectorInterventionEffectsApply : public ISupports
    {
    public:
        virtual void UpdateLarvalKilling( VectorHabitatType::Enum habitat, float killing ) = 0;
        virtual void UpdateLarvalHabitatReduction( VectorHabitatType::Enum habitat, float reduction ) = 0;
        virtual void UpdateOutdoorKilling( float killing ) = 0;
        virtual void UpdateOviTrapKilling(VectorHabitatType::Enum  habitat, float killing) = 0;
        virtual void UpdateVillageSpatialRepellent(float) = 0;
        virtual void UpdateADIVAttraction(float) = 0;
        virtual void UpdateADOVAttraction(float) = 0;
        virtual void UpdatePFVKill(float) = 0;
        virtual void UpdateOutdoorKillingMale(float) = 0;
        virtual void UpdateSugarFeedKilling(float) = 0;
        virtual void UpdateAnimalFeedKilling(float) = 0;
        virtual void UpdateOutdoorRestKilling(float) = 0;
    };

    class IMosquitoReleaseConsumer : public ISupports
    {
    public:
        virtual void ReleaseMosquitoes( NonNegativeFloat cost, const std::string& species, const VectorMatingStructure& genetics, NaturalNumber number ) = 0;
    };

    class NodeVectorEventContextHost :
        public NodeEventContextHost,
        public INodeVectorInterventionEffects,
        public INodeVectorInterventionEffectsApply,
        public IMosquitoReleaseConsumer
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()

    public:
        NodeVectorEventContextHost(Node* _node);
        virtual ~NodeVectorEventContextHost();
  
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance);
       
        // INodeVectorInterventionEffectsApply
        virtual void UpdateLarvalKilling( VectorHabitatType::Enum habitat, float killing );
        virtual void UpdateLarvalHabitatReduction( VectorHabitatType::Enum habitat, float reduction );
        virtual void UpdateOutdoorKilling( float killing );
        virtual void UpdateVillageSpatialRepellent(float reduction);
        virtual void UpdateADIVAttraction(float reduction);
        virtual void UpdateADOVAttraction(float reduction);
        virtual void UpdatePFVKill(float killing);
        virtual void UpdateOutdoorKillingMale(float killing);
        virtual void UpdateSugarFeedKilling(float killing);
        virtual void UpdateOviTrapKilling(VectorHabitatType::Enum  habitat, float killing);
        virtual void UpdateAnimalFeedKilling(float killing);
        virtual void UpdateOutdoorRestKilling(float killing);

        // INodeVectorInterventionEffects;
        virtual float GetLarvalKilling(VectorHabitatType::Enum);
        virtual float GetLarvalHabitatReduction(VectorHabitatType::Enum);
        virtual float GetVillageSpatialRepellent();
        virtual float GetADIVAttraction();
        virtual float GetADOVAttraction();
        virtual float GetPFVKill();
        virtual float GetOutdoorKilling();
        virtual float GetOutdoorKillingMale();
        virtual float GetSugarFeedKilling();
        virtual float GetOviTrapKilling(VectorHabitatType::Enum);
        virtual float GetAnimalFeedKilling();
        virtual float GetOutdoorRestKilling();

        VectorHabitatType::Enum larval_killing_target;
        VectorHabitatType::Enum larval_reduction_target;
        VectorHabitatType::Enum ovitrap_killing_target;

        // IMosquitoReleaseConsumer
        virtual void ReleaseMosquitoes( NonNegativeFloat cost, const std::string& species, const VectorMatingStructure& genetics, NaturalNumber number );

    protected: 
        float pLarvalKilling;
        float pLarvalHabitatReduction;
        float pVillageSpatialRepellent;
        float pADIVAttraction;
        float pADOVAttraction;
        float pPFVKill;
        float pOutdoorKilling;
        float pOutdoorKillingMale;
        float pSugarFeedKilling;
        float pOviTrapKilling;
        float pAnimalFeedKilling;
        float pOutdoorRestKilling;

    private:
        NodeVectorEventContextHost() : NodeEventContextHost(nullptr) { }
    };
}
