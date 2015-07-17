/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "NodeEventCoordinator.h"
#include "InterventionFactory.h"

static const char * _module = "NodeEventCoordinator";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(NodeEventCoordinator)
    IMPL_QUERY_INTERFACE2(NodeEventCoordinator, IEventCoordinator, IConfigurable)

    NodeEventCoordinator::NodeEventCoordinator()
    {
        // No base-class initialization stuff
    }

    QuickBuilder NodeEventCoordinator::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    bool NodeEventCoordinator::Configure(const Configuration* inputJson)
    {
        JsonConfigurable::_useDefaults = InterventionFactory::useDefaults;
        initializeInterventionConfig( inputJson );
        bool configured = JsonConfigurable::Configure( inputJson );
        JsonConfigurable::_useDefaults = false;
        return configured;
    }

    void NodeEventCoordinator::UpdateNodes( float dt )
    {
        // Intervention class names for informative logging
        std::ostringstream intervention_name;
        intervention_name << std::string( json::QuickInterpreter(intervention_config._json)["class"].As<json::String>() );

        // Simplest NDI distribution without repetition
        INodeDistributableIntervention *ndi = NULL;
        for(auto *nec : cached_nodes)
        {
			ndi = InterventionFactory::getInstance()->CreateNDIIntervention(Configuration::CopyFromElement(intervention_config._json));
            if(ndi)
            {
                if (!ndi->Distribute( nec, this ) )
                {
                    ndi->Release(); // a bit wasteful for now, could cache it for the next fellow
                }
                else
                {
                    LOG_INFO_F("UpdateNodes() distributed '%s' intervention to node %d\n", intervention_name.str().c_str(), nec->GetId().data );
                }
            }
            else
            {
                // add NDI-only exception
                std::string err = "Unable to create an instance of " + intervention_name.str() + " as an INodeDistributableIntervention.";
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, err.c_str());
            }
        }

        // Dispose
        if(ndi) ndi->Release();
        distribution_complete = true;
    }
}
