/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include "IdmApi.h"

namespace Kernel
{
    struct IInitialLoadBalanceScheme
    {
        virtual ~IInitialLoadBalanceScheme() {};

        virtual void Initialize( const std::string& rFilename, 
                                 uint32_t expectedNumNodes, 
                                 uint32_t numTasks ) = 0;

        virtual int GetInitialRankFromNodeId( uint32_t node_id ) = 0;
    };

    class IDMAPI LoadBalanceSchemeFactory
    {
    public:
        static IInitialLoadBalanceScheme* Create( const std::string& rFilename, 
                                                  uint32_t expectedNumNodes, 
                                                  uint32_t numTasks );
    };
}