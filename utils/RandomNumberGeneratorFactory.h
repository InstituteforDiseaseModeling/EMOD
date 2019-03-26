/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    class RANDOMBASE;

    ENUM_DEFINE(RandomNumberGeneratorType, 
        ENUM_VALUE_SPEC(USE_PSEUDO_DES          , 0)
        ENUM_VALUE_SPEC(USE_LINEAR_CONGRUENTIAL , 1)
        ENUM_VALUE_SPEC(USE_AES_COUNTER         , 2))

    ENUM_DEFINE(RandomNumberGeneratorPolicy, 
        ENUM_VALUE_SPEC(ONE_PER_CORE       , 0)
        ENUM_VALUE_SPEC(ONE_PER_NODE       , 1))
      //ENUM_VALUE_SPEC(ONE_PER_INDIVIDUAL , 2))

    class RandomNumberGeneratorFactory : public JsonConfigurable, public ISerializable
    {
    public:
        GET_SCHEMA_STATIC_WRAPPER( RandomNumberGeneratorFactory )
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        RandomNumberGeneratorFactory();
        ~RandomNumberGeneratorFactory();

        virtual bool Configure( const Configuration* inputJson ) override;

        RandomNumberGeneratorPolicy::Enum GetPolicy() const;

        void SetNodeIds( const std::vector<ExternalNodeId_t>& rNodeIds );

        RANDOMBASE* CreateRng( ExternalNodeId_t externalNodeId= UINT_MAX );

        void CreateFromSerializeData( bool fromSerializedData );

        DECLARE_SERIALIZABLE( RandomNumberGeneratorFactory );

    private:

        uint32_t CreateSeed( ExternalNodeId_t externalNodeId );
        RANDOMBASE* CreateRng( uint32_t seed, uint32_t cachedCount );
        uint32_t GetCachedCount();

        RandomNumberGeneratorType::Enum m_RngType;
        RandomNumberGeneratorPolicy::Enum m_Policy;
        bool m_CreateFromSerializedData;
        uint16_t m_RunNumber;
        std::vector<ExternalNodeId_t> m_NodeIds;
        uint32_t m_NodeIdsIndex;
        RANDOMBASE* m_pSeedRng;
    };
}