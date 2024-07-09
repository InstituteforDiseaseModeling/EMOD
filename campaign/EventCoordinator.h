
#pragma once

#include <string>
#include <list>
#include <vector>
#include "ISupports.h"
#include "Configuration.h"
#include "SimulationEventContext.h"
#include "RANDOM.h"
#include "InterventionEnums.h"
#include "FactorySupport.h"
#include "ObjectFactory.h"

namespace Kernel
{
    struct INodeEventContext;
    struct ISimulationEventContext;
    struct IEventCoordinator;
    struct IEventCoordinatorEventContext;
    struct IdmDateTime;

    namespace suids 
    {
        class suid ;
    }

    struct IDMAPI IEventCoordinator : public ISupports
    {
        virtual void SetContextTo(ISimulationEventContext *isec) = 0;
        virtual void CheckStartDay( float campaignStartDay ) const = 0;
        virtual void AddNode( const suids::suid& suid) = 0;
        virtual void Update(float dt) = 0;
        virtual void UpdateNodes(float dt) = 0;
        virtual bool IsFinished() = 0; // returns false when the EC requires no further updates and can be disposed of
        virtual IEventCoordinatorEventContext* GetEventContext() = 0;
    };

    struct IDMAPI IEventCoordinatorEventContext : public ISupports
    {
        virtual const std::string& GetName() const = 0;
        virtual const IdmDateTime& GetTime() const = 0;
        virtual IEventCoordinator* GetEventCoordinator() = 0;
    };

    struct IEventCoordinator2 : public ISupports
    {
        virtual float GetDemographicCoverage() const = 0;
        virtual TargetDemographicType::Enum GetTargetDemographic() const = 0;
        virtual float GetMinimumAge() const = 0;
        virtual float GetMaximumAge() const = 0;
    };

    class EventCoordinatorFactory : public ObjectFactory<IEventCoordinator,EventCoordinatorFactory>
    {
    };
}
