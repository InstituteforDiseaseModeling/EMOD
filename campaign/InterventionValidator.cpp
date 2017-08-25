/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include <stdafx.h>
#include "InterventionValidator.h"
#include "InterventionFactory.h"
#include "Contexts.h"
#include "Exceptions.h"

SETUP_LOGGING( "InterventionValidator" )

namespace Kernel
{
    void InterventionValidator::ValidateInterventionArray( const json::Element& rElement, const std::string& rDataLocation )
    {
        if( JsonConfigurable::_dryrun )
        {
            // skip check if getting schema
            return ;
        }

        const json::Array & interventions_array = json::QuickInterpreter( rElement ).As<json::Array>();
        LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());
        for( int idx=0; idx<interventions_array.Size(); idx++ )
        {
            const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[idx]);
            ValidateIntervention( actualIntervention, rDataLocation );
        }
    }

    void InterventionValidator::ValidateIntervention( const json::Element& rElement, const std::string& rDataLocation )
    {
        if( JsonConfigurable::_dryrun )
        {
            // skip check if getting schema
            return ;
        }

#if defined(_DLLS_)
        // For now just skip this test during dll builds: need to develop non-static solution.
        return;
#endif

        std::string class_name = std::string(json::QuickInterpreter(rElement)["class"].As<json::String>()) ;
        LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", class_name.c_str() );

        auto qi_as_config = Configuration::CopyFromElement( rElement, rDataLocation );
        INodeDistributableIntervention *ndi = InterventionFactory::getInstance()->CreateNDIIntervention(qi_as_config);

        std::string sim_type_str = GET_CONFIG_STRING(EnvPtr->Config, "Simulation_Type");
        if( ndi != nullptr )
        {
            ndi->ValidateSimType( sim_type_str );

            delete ndi ;
        }
        else
        {
            IDistributableIntervention *di = InterventionFactory::getInstance()->CreateIntervention(qi_as_config);
            if( di == nullptr )
            {
                std::stringstream ss ;
                ss << "Error creating the intervention '" << class_name << "'" ;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            di->ValidateSimType( sim_type_str );

            delete di ;
        }
        delete qi_as_config ;
    }
}
