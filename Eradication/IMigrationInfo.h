/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "stdafx.h"

#include <vector>

#include "suids.hpp"
#include "SimulationEnums.h"
#include "Configure.h"
#include "INodeContext.h"

class Configuration;


namespace Kernel
{
    struct IIndividualHumanContext;

    // IMigrationInfo is used to determine when and where an individual will migrate.
    struct IDMAPI IMigrationInfo
    {
        virtual ~IMigrationInfo() {};

        virtual void PickMigrationStep( IIndividualHumanContext * traveler, 
                                        float migration_rate_modifier, 
                                        suids::suid &destination, 
                                        MigrationType::Enum &migration_type,
                                        float &durationToWaitBeforeMigrating ) = 0;

        // needed for serialization
        virtual void SetContextTo(INodeContext* parent) = 0;

        virtual const std::vector<suids::suid>& GetReachableNodes() const = 0;
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypes() const = 0;

        virtual bool IsHeterogeneityEnabled() const = 0;
    };

    // IMigrationInfoFactory is used to create IMirationInfo objects for a node.
    struct IDMAPI IMigrationInfoFactory
    {
        virtual ~IMigrationInfoFactory() {};

        virtual bool Configure( const Configuration* config ) = 0;

        virtual void Initialize( const ::Configuration *config, const std::string& idreference ) = 0;
        virtual IMigrationInfo* CreateMigrationInfo( INodeContext *parent_node, 
                                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) = 0;

        virtual bool IsAtLeastOneTypeConfiguredForIndividuals() const = 0;
        virtual bool IsEnabled( MigrationType::Enum mt ) const = 0;
    };

    namespace MigrationFactory
    {
        // Contructs and initializes the proper factory based on the initialization parameters
        IMigrationInfoFactory IDMAPI * ConstructMigrationInfoFactory( const ::Configuration *config, 
                                                                      const std::string& idreference,
                                                                      SimType::Enum sim_type,
                                                                      MigrationStructure::Enum ms,
                                                                      bool useDefaultMigration,
                                                                      int defaultTorusSize=10 );
    }
}
