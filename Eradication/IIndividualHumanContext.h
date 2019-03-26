/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

// put all contexts in one place to reduce clutter in includes
#pragma once
#include <list>
#include <vector>
#include "suids.hpp"
#include "ISupports.h"
#include "IdmApi.h"

namespace Kernel
{
    class RANDOMBASE;
    struct IIndividualHumanInterventionsContext;
    struct IIndividualHumanEventContext;
    struct ISusceptibilityContext;
    struct NodeDemographics;
    struct IInfection;

    struct IIndividualHumanContext : ISupports
    {
        virtual suids::suid GetSuid() const = 0;

        virtual suids::suid GetNextInfectionSuid() = 0;
        virtual RANDOMBASE* GetRng() = 0;

        virtual IIndividualHumanInterventionsContext *GetInterventionsContext() const = 0; // internal components of individuals interact with interventions via this interface

        virtual IIndividualHumanInterventionsContext *GetInterventionsContextbyInfection( IInfection* infection ) = 0; // internal components of individuals interact with interventions via this interface
        virtual IIndividualHumanEventContext *GetEventContext() = 0;                       // access to specific attributes of the individual useful for events
        virtual ISusceptibilityContext *GetSusceptibilityContext() const = 0;              // access to immune attributes useful for infection, interventions, reporting, etc.

        virtual const NodeDemographics* GetDemographics() const = 0;

        virtual void UpdateGroupMembership() = 0;
        virtual void UpdateGroupPopulation( float size_changes ) = 0;

        virtual const std::string& GetPropertyReportString() const = 0;
        virtual void SetPropertyReportString( const std::string& str ) = 0;
    };
}
