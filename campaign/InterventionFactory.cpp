/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#ifdef WIN32
#include "windows.h" // why oh why do I suddenly have to include this???
#else
#include <dirent.h>
#include <dlfcn.h>
#endif
#include "Sugar.h"

#include "InterventionFactory.h"
#include "Interventions.h"
#include "Log.h"

SETUP_LOGGING( "InterventionFactory" )

json::Object Kernel::InterventionFactory::campaignSchema;

namespace Kernel
{
    bool InterventionFactory::useDefaults = false; // stores value from campaign.json
    // Technically this should be in its own file, BaseIntervention.cpp, but I couldn't bring myself to 
    // create such a miniscule file, so I talked myself into putting it here. :)
    IInterventionFactory* InterventionFactory::_instance = nullptr;

    // ctor
    InterventionFactory::InterventionFactory()
    {
        if( _instance != nullptr )
        {
            throw std::runtime_error( "Second InterventionFactory being created." );
        }
    }

    void InterventionFactory::LoadDynamicLibraries()
    {
    }

    // new, configurable method
    IDistributableIntervention* InterventionFactory::CreateIntervention( const Configuration *config )
    {
        // Keeping this simple. But bear in mind CreateInstanceFromSpecs can throw exception
        // and JC::_useDefaults will not be restored. But we won't keep running in that case.
        bool reset = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = useDefaults;
        IDistributableIntervention* ret = CreateInstanceFromSpecs<IDistributableIntervention>(config, getRegisteredClasses(), true);
        JsonConfigurable::_useDefaults = reset;
        return ret;
    }

    INodeDistributableIntervention* InterventionFactory::CreateNDIIntervention( const Configuration *config )
    {
        bool reset = JsonConfigurable::_useDefaults;
        JsonConfigurable::_useDefaults = useDefaults;
        INodeDistributableIntervention* ret = CreateInstanceFromSpecs<INodeDistributableIntervention>(config, getRegisteredClasses(), true);
        JsonConfigurable::_useDefaults = reset;
        return ret;
    }

    json::QuickBuilder
    InterventionFactory::GetSchema()
    {
        // Iterate over all registrangs, instantiate using function pointer (or functor?), call Configure
        // but with special 'don't error out' mode.
        //json::Object returnVal;
        //campaignSchema["hello"] = json::String( "world" );
        support_spec_map_t& registrants = getRegisteredClasses();
#ifdef WIN32
        JsonConfigurable::_dryrun = true;
#else
        setenv( "DRYRUN", "1", 1 );
#endif
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
                ISupports * pCampaignForSchema = CreateIntervention((const Configuration*)fakeConfig);
                if( pCampaignForSchema )
                {
                    assert( pCampaignForSchema );
                    json::QuickBuilder* schema = &dynamic_cast<JsonConfigurable*>(pCampaignForSchema)->GetSchema();
                    (*schema)[std::string("iv_type")] = json::String("IndividualTargeted");
                    (*schema)[std::string("class")] = json::String(class_name);
                    campaignSchema[ class_name ] = *schema;
                }
                else // Try Node-targeted
                {
                    ISupports * pCampaignForSchema = CreateNDIIntervention((const Configuration*)fakeConfig);
                    json::QuickBuilder* schema = &dynamic_cast<JsonConfigurable*>(pCampaignForSchema)->GetSchema();
                    (*schema)[std::string("iv_type")] = json::String("NodeTargeted");
                    (*schema)[std::string("class")] = json::String(class_name);
                    campaignSchema[ class_name ] = *schema;
                }
            }
            catch( DetailedException &e )
            {
                std::ostringstream msg;
                msg << "ConfigException creating intervention for GetSchema: " 
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            catch( const json::Exception &e )
            {
                std::ostringstream msg;
                msg << "json Exception creating intervention for GetSchema: "
                    << e.what()
                    << std::endl;
                LOG_INFO( msg.str().c_str() );
            }
            LOG_DEBUG( "Done with that class....\n" );
            delete fakeConfig;
            fakeConfig = nullptr;
        }
        LOG_DEBUG( "Returning from GetSchema.\n" );
        json::QuickBuilder retSchema = json::QuickBuilder(campaignSchema);
        return retSchema;
    }
}
