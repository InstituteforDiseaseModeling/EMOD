/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <map>

#include "ISupports.h"
#include "NodeEventContext.h"
#include "NodeEventContextHost.h"
#include "TyphoidDefs.h"
#include "TyphoidInterventionsContainer.h"
#include "Types.h"

namespace Kernel
{
    class Simulation;

    struct INodeTyphoidInterventionEffectsApply : public ISupports
    {
        virtual void ApplyReducedSheddingEffect( float rate, const std::string& properties_raw ) = 0;
        virtual void ApplyReducedDoseEffect( float rate, const std::string& properties_raw ) = 0;
        virtual void ApplyReducedNumberExposuresEffect( float rate, const std::string& properties_raw ) = 0;
    };

    struct INodeTyphoidInterventionEffects : public ISupports
    {
        virtual float GetEnviroDepositAttenuation( const IPKeyValueContainer * props = nullptr ) const = 0;
        virtual float GetEnviroExposuresAttenuation( const IPKeyValueContainer * props = nullptr ) const = 0;
        virtual float GetEnviroDoseAttenuation( const IPKeyValueContainer * props = nullptr ) const = 0;
    };

    // The purpose of this class is to the typhoid-specific interventions container for node-level interventions.
    // Like all interventions containers, it mediates between individuals and interventions via interventions-effects.
    // The first node-level typyoid-specific intervention is the TyphoidWASH, but the container shouldn't have any
    // intervention-specific hard-coded.  
    class NodeTyphoidEventContextHost :
        public NodeEventContextHost,
        public INodeTyphoidInterventionEffectsApply,
        public INodeTyphoidInterventionEffects
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()

    public:
        NodeTyphoidEventContextHost(Node* _node);
        virtual ~NodeTyphoidEventContextHost();
 
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // INodeTyphoidInterventionEffectsApply: these are methods called by the intervention to effect change.  
        virtual void ApplyReducedSheddingEffect( float rate, const std::string& properties_raw ) override;
        virtual void ApplyReducedDoseEffect( float rate, const std::string& properties_raw ) override;
        virtual void ApplyReducedNumberExposuresEffect( float rate, const std::string& properties_raw ) override;
        
        // INodeTyphoidInterventionEffects: these are methods called by the node (actually individual!) to 
        // discover change effects.
        // NOTE: normally node-level intervention effects get applied to the node, but in this case the effects
        // are held by the node but applied at the individual level.
        virtual float GetEnviroDepositAttenuation( const IPKeyValueContainer * props = nullptr ) const override;
        virtual float GetEnviroExposuresAttenuation( const IPKeyValueContainer * props = nullptr ) const override;
        virtual float GetEnviroDoseAttenuation( const IPKeyValueContainer * props = nullptr ) const override;
     
    protected: 

    private:
        NodeTyphoidEventContextHost() : NodeEventContextHost(nullptr) { }
        // We have maps because the effects operate on individual properties normally, and these are strings. 
        std::map< std::string, ProbabilityNumber > current_shedding_attenuation_environment;
        std::map< std::string, ProbabilityNumber > current_dose_attenuation_environment;
        std::map< std::string, ProbabilityNumber > current_exposures_attenuation_environment;
    };
}
