/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "ScaleLarvalHabitat.h"
#include "NodeVectorEventContext.h" // for INodeVectorInterventionEffectsApply methods

static const char * _module = "ScaleLarvalHabitat";

namespace Kernel
{

    IMPLEMENT_FACTORY_REGISTERED(ScaleLarvalHabitat)

    void ScaleLarvalHabitat::Update( float dt )
    {
        // Do not decay the scaled habitat,
        // although it can be overriden by another Distribute.

        // Just push its effects to the NodeEventContext
        ApplyEffects();
    }

    bool ScaleLarvalHabitat::Configure( const Configuration * inputJson )
    {
        initConfig( "Habitat_Target", 
                    habitat_target, 
                    inputJson, 
                    MetadataDescriptor::Enum( "Habitat_Target", 
                                              SLH_Habitat_Target_DESC_TEXT, 
                                              MDD_ENUM_ARGS(VectorHabitatType) 
                                            ) 
                  );

        initConfigTypeMap("Habitat_Scale", &reduction, SLH_Habitat_Scale_DESC_TEXT, 0, 10, 1);

        return JsonConfigurable::Configure( inputJson );
    }

    void ScaleLarvalHabitat::ApplyEffects()
    {
        if( invic )
        {
            // Use habitat reduction function, hence (1-Habitat_Scale)
            invic->UpdateLarvalHabitatReduction( GetHabitatTarget(), 
                                                 1.0f - GetReduction() );
        }
        else
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, 
                                        "invic", "INodeVectorInterventionEffectsApply" );
        }
    }
}