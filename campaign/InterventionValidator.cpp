/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

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
    InterventionTypeValidation::Enum 
        InterventionValidator::ValidateInterventionArray( const std::string& rOwnerName,
                                                          InterventionTypeValidation::Enum requiredType,
                                                          const json::Element& rElement,
                                                          const std::string& rDataLocation )
    {
        release_assert( requiredType != InterventionTypeValidation::UNKNOWN );

        if( JsonConfigurable::_dryrun )
        {
            // skip check if getting schema
            return InterventionTypeValidation::UNKNOWN;
        }

        InterventionTypeValidation::Enum all_of_type = requiredType;
        if( all_of_type == InterventionTypeValidation::EITHER )
        {
            all_of_type = InterventionTypeValidation::UNKNOWN;
        }

        const json::Array & interventions_array = json::QuickInterpreter( rElement ).As<json::Array>();
        LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());
        for( int idx=0; idx<interventions_array.Size(); idx++ )
        {
            const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[idx]);
            InterventionTypeValidation::Enum found_type = ValidateIntervention( rOwnerName, requiredType, actualIntervention, rDataLocation );
            if( all_of_type == InterventionTypeValidation::UNKNOWN )
            {
                all_of_type = found_type;
            }
            else if( all_of_type != found_type )
            {
                // NOTE: I should only get this far if requiredType is EITHER.
                std::stringstream ss;
                ss << "Invalid mixing of intervention types in '" << rOwnerName << "'.  Arrays of interventions must either be all node-level or all individual-level.";
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
        }
        return all_of_type;
    }

    InterventionTypeValidation::Enum
        InterventionValidator::ValidateIntervention( const std::string& rOwnerName,
                                                     InterventionTypeValidation::Enum requiredType,
                                                     const json::Element& rElement,
                                                     const std::string& rDataLocation )
    {
        release_assert( requiredType != InterventionTypeValidation::UNKNOWN );

        if( JsonConfigurable::_dryrun )
        {
            // skip check if getting schema
            return InterventionTypeValidation::UNKNOWN;
        }

#if defined(_DLLS_)
        // For now just skip this test during dll builds: need to develop non-static solution.
        return;
#endif

        InterventionTypeValidation::Enum found_type = InterventionTypeValidation::UNKNOWN;

        std::string class_name = std::string(json::QuickInterpreter(rElement)["class"].As<json::String>()) ;
        LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", class_name.c_str() );

        auto qi_as_config = Configuration::CopyFromElement( rElement, rDataLocation );
        INodeDistributableIntervention *ndi = InterventionFactory::getInstance()->CreateNDIIntervention(qi_as_config);

        std::string sim_type_str = GET_CONFIG_STRING(EnvPtr->Config, "Simulation_Type");
        if( ndi != nullptr )
        {
            ndi->ValidateSimType( sim_type_str );

            delete ndi ;

            found_type = InterventionTypeValidation::NODE;
        }
        else
        {
            IDistributableIntervention *di = InterventionFactory::getInstance()->CreateIntervention(qi_as_config);
            if( di == nullptr )
            {
                std::stringstream ss ;
                ss << "Error creating the intervention '" << class_name << "' in '" << rOwnerName << "'" ;
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            di->ValidateSimType( sim_type_str );

            delete di ;

            found_type = InterventionTypeValidation::INDIVIDUAL;
        }
        delete qi_as_config ;

        if( (requiredType != InterventionTypeValidation::EITHER) && (requiredType != found_type) )
        {
            std::string required_type_str = InterventionTypeValidation::pairs::lookup_key( requiredType );
            std::string found_type_str    = InterventionTypeValidation::pairs::lookup_key( found_type   );
            std::stringstream ss;
            ss << "Invalid Intervention Type in '" << rOwnerName <<"'." << std::endl;
            ss << "'" << class_name << "' is a(n) " << found_type_str << "-level intervention." << std::endl;
            ss << "A(n) " << required_type_str << "-level intervention is required.";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return found_type;
    }
}
