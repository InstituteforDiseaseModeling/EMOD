
#pragma once

#include "stdafx.h"
#include "IMigrationInfo.h"
#include "SimulationEnums.h"
#include "VectorEnums.h"


namespace Kernel
{
    struct IVectorSimulationContext;

    // Extend the IMigrationInfo interface to support migrating vectors
    struct IMigrationInfoVector : virtual IMigrationInfo
    {
        virtual ~IMigrationInfoVector() {};

        // The rates for vectors change change due to the 
        // change in human population and vector habitat
        virtual void UpdateRates( const suids::suid& rThisNodeId, 
                                  const std::string& rSpeciesID, 
                                  IVectorSimulationContext* pivsc) = 0;

        virtual Gender::Enum ConvertVectorGender(VectorGender::Enum vector_gender) const = 0;
        virtual void CalculateRates(VectorGender::Enum vector_gender) = 0;
    };

    struct IMigrationInfoFactoryVector
    {
        virtual ~IMigrationInfoFactoryVector() {};

        virtual void ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config ) = 0;
        virtual IMigrationInfoVector* CreateMigrationInfoVector(
            const std::string& idreference,
            INodeContext *parent_node, 
            const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) = 0;
    };
}
