/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "IdmApi.h"
#include "EnumSupport.h"
#include "Types.h"
#include <set>
#include <functional>

#include "suids.hpp"

namespace Kernel
{
    struct IIndividualHumanSTI;
    class RelationshipManager;
    typedef std::set< IIndividualHumanSTI* > tRelationshipMembers;

    ENUM_DEFINE(RelationshipType,
        ENUM_VALUE_SPEC(TRANSITORY                  , 0)
        ENUM_VALUE_SPEC(INFORMAL                    , 1)
        ENUM_VALUE_SPEC(MARITAL                     , 2)
        ENUM_VALUE_SPEC(COUNT                       , 3))

    struct IRelationshipManager;

    struct IDMAPI IRelationship
    {
        virtual IIndividualHumanSTI* MalePartner() const = 0;
        virtual IIndividualHumanSTI* FemalePartner() const = 0;
        virtual IIndividualHumanSTI* GetPartner( IIndividualHumanSTI* pIndiv ) const = 0;
        virtual unsigned long int GetId() const = 0;
        virtual void SetSuid(suids::suid) = 0;
        virtual suids::suid GetSuid() = 0;
        virtual void Consummate( float dt ) = 0;
        virtual const std::string& GetPropertyKey() = 0;
        virtual const std::string& GetPropertyName() const = 0;
        virtual const tRelationshipMembers GetMembers() const = 0;
        virtual void pause( IIndividualHumanSTI* departee ) = 0;
        virtual void resume( IIndividualHumanSTI* returnee ) = 0;
        virtual void terminate( IRelationshipManager* ) = 0;
        virtual bool IsDiscordant() const = 0;
        virtual void Initialize( IRelationshipManager* ) = 0;
        virtual bool Update( float dt ) = 0;
        virtual float GetDuration() const = 0;
        virtual float GetTimer() const = 0;
        virtual float GetStartTime() const = 0;
        virtual float GetScheduledEndTime() const = 0;
        virtual bool GetUsingCondom() const = 0;
        virtual suids::suid GetMalePartnerId() const = 0;
        virtual suids::suid GetFemalePartnerId() const = 0;
        virtual RelationshipType::Enum GetType() const = 0;
        virtual suids::suid GetOriginalNodeId() const = 0;
        virtual float GetCoitalRate() const = 0;

        virtual ~IRelationship() {}
    };

    typedef std::function<void(IIndividualHumanSTI*,IIndividualHumanSTI*)> RelationshipCreator;
}
