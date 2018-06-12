/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "WaningEffect.h"
#include "CajunIncludes.h"
#include "ConfigurationImpl.h"

json::Object Kernel::WaningEffectFactory::campaignSchema;

SETUP_LOGGING( "WaningEffectFactory" )

namespace Kernel
{
    json::QuickBuilder WaningEffectFactory::GetSchema()
    {
        // TODO: put repeated factory-schema pattern into FactorySupport.h?
        // Iterate over all registrants, instantiate using function pointer (or functor?), call Configure
        // but with special 'don't error out' mode.
        support_spec_map_t& registrants = getRegisteredClasses();
#ifdef WIN32
        JsonConfigurable::_dryrun = true;
#else
        setenv( "DRYRUN", "1", 1 );
#endif
        json::Object waningEffectsJsonArray;
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
                ISupports * pNS = CreateInstanceFromSpecs<IWaningEffect>(fakeConfig, getRegisteredClasses(), true);
                if( pNS )
                {
                    json::QuickBuilder* schema = &dynamic_cast<JsonConfigurable*>(pNS)->GetSchema();
                    (*schema)[ "class" ] = json::String( class_name );
                    waningEffectsJsonArray[ class_name ] = *schema;
                }
            }
            catch( DetailedException &e )
            {
                std::ostringstream msg;
                msg << "ConfigException creating WaningEffect for GetSchema: " 
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            catch( const json::Exception &e )
            {
                std::ostringstream msg;
                msg << "json Exception creating WaningEffect for GetSchema: "
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            LOG_DEBUG( "Done with that class....\n" );
            delete fakeConfig;
            fakeConfig = nullptr;
        }
        campaignSchema[ "schema" ] = waningEffectsJsonArray;
        LOG_DEBUG( "Returning from GetSchema.\n" );
        json::QuickBuilder retSchema = json::QuickBuilder(campaignSchema);
        return retSchema;
    }

    IWaningEffectFactory * WaningEffectFactory::_instance = nullptr;
}