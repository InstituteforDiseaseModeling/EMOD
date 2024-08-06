
#pragma once

#include "stdafx.h"

#include <vector>

#include "suids.hpp"
#include "SimulationEnums.h"
#include "Configure.h"
#include "ExternalNodeId.h"

class Configuration;


namespace Kernel
{
    class RANDOMBASE;
    struct IIndividualHumanEventContext;
    struct INodeContext;

    // IMigrationInfo is used to determine when and where an individual will migrate.
    struct IMigrationInfo
    {
        virtual ~IMigrationInfo() {};

        virtual void PickMigrationStep( RANDOMBASE* pRNG,
                                        IIndividualHumanEventContext * traveler, 
                                        float migration_rate_modifier, 
                                        suids::suid &destination, 
                                        MigrationType::Enum &migration_type,
                                        float &durationToWaitBeforeMigrating ) = 0;

        // needed for serialization
        virtual void SetContextTo(INodeContext* parent) = 0;

        virtual float GetTotalRate() const = 0;
        virtual const std::vector<float>& GetCumulativeDistributionFunction() const = 0;
        virtual const std::vector<suids::suid>& GetReachableNodes() const = 0;
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypes() const = 0;
        virtual const std::vector<suids::suid>& GetReachableNodes(Gender::Enum gender) const = 0;
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypes(Gender::Enum gender) const = 0;

        virtual bool IsHeterogeneityEnabled() const = 0;
    };

    // IMigrationInfoFactory is used to create IMirationInfo objects for a node.
    struct IMigrationInfoFactory
    {
        virtual ~IMigrationInfoFactory() {};

        virtual bool Configure( const Configuration* config ) = 0;

        virtual void Initialize( const ::Configuration *config, const std::string& idreference ) = 0;
        virtual IMigrationInfo* CreateMigrationInfo( INodeContext *parent_node, 
                                                     const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) = 0;

        virtual bool IsAtLeastOneTypeConfiguredForIndividuals() const = 0;
        virtual bool IsEnabled( MigrationType::Enum mt ) const = 0;
    };

#ifndef DISABLE_VECTOR
    struct IMigrationInfoFactoryVector;
#endif

    namespace MigrationFactory
    {
        // Contructs and initializes the proper factory based on the initialization parameters
        IMigrationInfoFactory* ConstructMigrationInfoFactory( const ::Configuration *config, 
                                                              const std::string& idreference,
                                                              SimType::Enum sim_type,
                                                              MigrationStructure::Enum ms,
                                                              bool useDefaultMigration,
                                                              int defaultTorusSize=10 );

#ifndef DISABLE_VECTOR
        IMigrationInfoFactoryVector* ConstructMigrationInfoFactoryVector( JsonConfigurable* pParent, 
                                                                          const ::Configuration *config );
#endif
    }
}
