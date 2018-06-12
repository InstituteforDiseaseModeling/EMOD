/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "NodeSet.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"
#include "InterventionEnums.h"

json::Object Kernel::NodeSetFactory::campaignSchema;

SETUP_LOGGING( "NodeSetFactory" )

namespace Kernel
{
    json::QuickBuilder
    NodeSetFactory::GetSchema()
    {
        // Iterate over all registrangs, instantiate using function pointer (or functor?), call Configure
        // but with special 'don't error out' mode.
        //json::Object returnVal;
        support_spec_map_t& registrants = getRegisteredClasses();

        JsonConfigurable::_dryrun = true;

        json::Object nodeSetsJsonArray;
        for (auto& entry : registrants)
        {
            const std::string& class_name = entry.first;
            LOG_DEBUG_F("class_name = %s\n", class_name.c_str());
            json::Object fakeJson;
            fakeJson["class"] = json::String(class_name);
            Configuration * fakeConfig = Configuration::CopyFromElement( fakeJson );
            instantiator_function_t creator = entry.second;

            try
            {
                // TBD: Handle Node-targeted interventions
                 
                ISupports * pNS = CreateInstanceFromSpecs<INodeSet>(fakeConfig, getRegisteredClasses(), true);
                if( pNS )
                {
                    //campaignSchema[std::string(class_name)] = dynamic_cast<JsonConfigurable*>(pNS)->GetSchema();
                    json::QuickBuilder* schema = &dynamic_cast<JsonConfigurable*>(pNS)->GetSchema();
                    (*schema)[ "class" ] = json::String( class_name );
                    nodeSetsJsonArray[ class_name ] = *schema;
                    //nodeSetsJsonArray[jsonArrayIdx][ "class" ] = json::String( class_name );
                }
            }
            catch( DetailedException &e )
            {
                std::ostringstream msg;
                msg << "ConfigException creating nodeset for GetSchema: " 
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            catch( const json::Exception &e )
            {
                std::ostringstream msg;
                msg << "json Exception creating nodeset for GetSchema: "
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            LOG_DEBUG( "Done with that class....\n" );
            delete fakeConfig;
            fakeConfig = nullptr;
        }
        campaignSchema[ "schema" ] = nodeSetsJsonArray;
        LOG_DEBUG( "Returning from GetSchema.\n" );
        json::QuickBuilder retSchema = json::QuickBuilder(campaignSchema);
        return retSchema;
    }

    // NodeSet
    INodeSetFactory * NodeSetFactory::_instance = nullptr;
}
