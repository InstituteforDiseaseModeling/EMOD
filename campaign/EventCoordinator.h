/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>
#include "ISupports.h"
#include "Configuration.h"
#include "RANDOM.h"
#include "SimulationEventContext.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"

namespace Kernel
{
    struct INodeEventContext;
    struct ISimulationEventContext;
    struct ISimulationEventContext;

    namespace suids 
    {
        class suid ;
    }

    struct IDMAPI IEventCoordinator : public ISupports
    {
        virtual void SetContextTo(ISimulationEventContext *isec) = 0;
        virtual void AddNode( const suids::suid& suid) = 0;
        virtual void Update(float dt) = 0;
        virtual void UpdateNodes(float dt) = 0;
        virtual bool IsFinished() = 0; // returns false when the EC requires no further updates and can be disposed of
    };

    struct IEventCoordinator2 : public ISupports
    {
        virtual float GetDemographicCoverage() const = 0;
        virtual TargetDemographicType::Enum GetTargetDemographic() const = 0;
        virtual float GetMinimumAge() const = 0;
        virtual float GetMaximumAge() const = 0;
    };

    class IEventCoordinatorFactory
    {
    public:
        virtual void Register(string classname, instantiator_function_t _if) = 0;
        virtual void Register(const char* classname, instantiator_function_t _if) = 0;
        virtual json::QuickBuilder GetSchema() = 0;
    };

    class EventCoordinatorFactory : public IEventCoordinatorFactory
    {
    public:
        static IEventCoordinatorFactory * getInstance()
        {
            return _instance ? _instance : _instance = new EventCoordinatorFactory();
        }

        static IEventCoordinator* CreateInstance(const Configuration* config); // instantiate from classname

        void Register(string classname, instantiator_function_t _if)
        {
            getRegisteredClasses()[classname] = _if;
        }

        void Register(const char* classname, instantiator_function_t _if)
        {
            getRegisteredClasses()[classname] = _if;
        }

        virtual json::QuickBuilder GetSchema();

    protected:
        static support_spec_map_t& getRegisteredClasses()
        {
            static support_spec_map_t registered_classes;
            return registered_classes;
        }

        static bool doThisOnce;
        static json::Object ecSchema;

    private:
        static IEventCoordinatorFactory * _instance;
    };
}
