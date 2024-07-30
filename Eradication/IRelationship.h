
#pragma once
#include "IdmApi.h"
#include "EnumSupport.h"
#include "Types.h"
#include <set>
#include <functional>

#include "suids.hpp"
#include "RelationshipType.h"

namespace Kernel
{
    class RANDOMBASE;
    struct IRelationshipManager;
    struct ISociety;
    struct Sigmoid;

    struct IIndividualHumanSTI;
    typedef std::set< IIndividualHumanSTI* > tRelationshipMembers;

#define MAX_SLOTS (63)

    ENUM_DEFINE(RelationshipState,
        ENUM_VALUE_SPEC( NORMAL      , 0)
        ENUM_VALUE_SPEC( PAUSED      , 1)
        ENUM_VALUE_SPEC( MIGRATING   , 2)
        ENUM_VALUE_SPEC( TERMINATED  , 3))

    ENUM_DEFINE(RelationshipMigrationAction,
        ENUM_VALUE_SPEC( PAUSE     , 0)
        ENUM_VALUE_SPEC( MIGRATE   , 1)
        ENUM_VALUE_SPEC( TERMINATE , 2))

    ENUM_DEFINE(RelationshipTerminationReason,
        ENUM_VALUE_SPEC( NA    , 0)
        ENUM_VALUE_SPEC( BROKEUP            , 1)
        ENUM_VALUE_SPEC( SELF_MIGRATING     , 2)
        ENUM_VALUE_SPEC( PARTNER_DIED       , 3)
        ENUM_VALUE_SPEC( PARTNER_TERMINATED , 4)
        ENUM_VALUE_SPEC( PARTNER_MIGRATING  , 5))

    struct IDMAPI IRelationship : ISerializable
    {
        virtual bool IsOutsidePFA() const = 0;
        virtual RelationshipState::Enum GetState() const = 0;
        virtual RelationshipState::Enum GetPreviousState() const = 0;
        virtual RelationshipMigrationAction::Enum GetMigrationAction( RANDOMBASE* prng ) const =0;
        virtual RelationshipTerminationReason::Enum GetTerminationReason() const = 0;
        virtual IRelationship* Pause( IIndividualHumanSTI* departee, const suids::suid& rMigrationDestination ) = 0;
        virtual void Terminate( RelationshipTerminationReason::Enum terminationReason ) = 0;
        virtual void Migrate( const suids::suid& rMigrationDestination ) = 0;
        virtual void Resume( ISociety* pSociety, IIndividualHumanSTI* returnee ) = 0;
        virtual void UpdatePaused() = 0;

        virtual bool Update( float dt ) = 0;
        virtual void Consummate( RANDOMBASE* pRNG, float dt, IRelationshipManager * relMan ) = 0;

        virtual IIndividualHumanSTI* MalePartner() const = 0;
        virtual IIndividualHumanSTI* FemalePartner() const = 0;
        virtual IIndividualHumanSTI* GetPartner( IIndividualHumanSTI* pIndiv ) const = 0;
        virtual const suids::suid& GetSuid() const = 0;
        virtual unsigned int GetSlotNumberForPartner( bool forPartnerB ) const = 0;
        virtual const tRelationshipMembers GetMembers() const = 0;
        virtual bool IsDiscordant() const = 0;
        virtual float GetDuration() const = 0;
        virtual float GetTimer() const = 0;
        virtual float GetStartTime() const = 0;
        virtual float GetScheduledEndTime() const = 0;
        virtual suids::suid GetPartnerId( const suids::suid& myID ) const = 0;
        virtual suids::suid GetMalePartnerId() const = 0;
        virtual suids::suid GetFemalePartnerId() const = 0;
        virtual bool IsMalePartnerAbsent() const = 0;
        virtual bool IsFemalePartnerAbsent() const = 0;

        virtual RelationshipType::Enum GetType() const = 0;
        virtual unsigned int GetOriginalNodeId() const = 0;
        virtual float GetCoitalRate() const = 0;
        virtual ProbabilityNumber getProbabilityUsingCondomThisAct() const =0;
        virtual unsigned int GetTotalCoitalActs() const = 0;
        virtual unsigned int GetNumCoitalActs() const = 0;
        virtual unsigned int GetNumCoitalActsUsingCondoms() const = 0;
        virtual bool HasMigrated() const = 0;
        virtual suids::suid GetMigrationDestination() const = 0;

        virtual ~IRelationship() {}
    };

    typedef std::function<void(IIndividualHumanSTI*,IIndividualHumanSTI*,bool,Sigmoid*)> RelationshipCreator;
}
