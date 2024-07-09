
#include "stdafx.h"
#include "ScaleLarvalHabitat.h"
#include "NodeVectorEventContext.h" // for INodeVectorInterventionEffectsApply methods

SETUP_LOGGING( "ScaleLarvalHabitat" )

namespace Kernel
{

    IMPLEMENT_FACTORY_REGISTERED(ScaleLarvalHabitat)

    ScaleLarvalHabitat::ScaleLarvalHabitat()
    : SimpleVectorControlNode()
    , m_LHM(true)
    {
    }

    ScaleLarvalHabitat::ScaleLarvalHabitat( const ScaleLarvalHabitat& master )
    : SimpleVectorControlNode( master )
    , m_LHM( master.m_LHM )
    {
    }

    ScaleLarvalHabitat::~ScaleLarvalHabitat()
    {
    }

    bool ScaleLarvalHabitat::Configure( const Configuration * inputJson )
    {
        m_LHM.Initialize();

        initConfigComplexCollectionType("Larval_Habitat_Multiplier", &m_LHM, SLH_Larval_Habitat_Multiplier_DESC_TEXT);
    
        // Don't call subclass SimpleVectorControlNode::Configure() because 
        // it will add cost_per_unit plus other stuff
        bool is_configured = BaseNodeIntervention::Configure( inputJson );
        if( is_configured && !JsonConfigurable::_dryrun )
        {
            m_LHM.CheckConfiguration();
        }
        return is_configured;
    }

    void ScaleLarvalHabitat::ApplyEffects( float dt )
    {
        release_assert( m_pINVIC != nullptr );
        m_pINVIC->UpdateLarvalHabitatReduction( m_LHM );
    }
}