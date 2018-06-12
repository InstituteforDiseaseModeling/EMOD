/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <functional>
#include "FactorySupport.h"
#include "Interventions.h"

namespace Kernel
{
    //struct IDistributableIntervention;

    // You can create 3 kinds of interventions:
    // 1) 'Regular' Individual-Targeted Interventions: IDistributableIntervention.
    // 2) 'Regular' Node-Targeted Interventions: INodeDistributableIntervention.
    // 3) 'Distributing' Individual-Targeted Interventions, e.g., Health-Seeking 
    // or Health-Targeted, which themselves actually distribute 1 or more actual 
    // interventions: IDistributingDistributableIntervention.
    class IInterventionFactory
    {
    public:
        virtual void Register(const char * classname, instantiator_function_t _if) = 0;
        virtual IDistributableIntervention* CreateIntervention(const Configuration *config) = 0; // returns NULL if could not create a distributable intervention with the specified definition
        virtual INodeDistributableIntervention* CreateNDIIntervention(const Configuration *config) = 0; // returns NULL if could not create a node distributable intervention with the specified definition
        virtual json::QuickBuilder GetSchema() = 0;
    };
            
    class InterventionFactory : public IInterventionFactory
    {
    public:

        static IInterventionFactory * getInstance()
        {
            return _instance ? _instance : _instance = new InterventionFactory();
        }

        IDistributableIntervention* CreateIntervention(const Configuration *config); // returns NULL if could not create a distributable intervention with the specified definition
        INodeDistributableIntervention* CreateNDIIntervention(const Configuration *config); // returns NULL if could not create a node distributable intervention with the specified definition

        // use static classes that call this registration method on init to get started           
        /*static*/ void Register(const char *classname, instantiator_function_t _if)
        {
            string classnameString(classname);
            getRegisteredClasses()[classnameString] = _if;
        }

        virtual json::QuickBuilder GetSchema();

        static bool useDefaults;

    protected:
        
        static support_spec_map_t& getRegisteredClasses()
        {
            static support_spec_map_t registered_classes;
            return registered_classes;
        }

        static json::Object campaignSchema;

    private:
        static IInterventionFactory * _instance;
        InterventionFactory();
        void LoadDynamicLibraries();
    };
}
