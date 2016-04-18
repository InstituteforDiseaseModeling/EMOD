/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "MathFunctions.h"
#include "IdmApi.h"

namespace Kernel
{
    struct IArchive;

    class IDMAPI DurationDistribution
    {
    public:
        DurationDistribution( DistributionFunction::Enum defaultType = DistributionFunction::NOT_INITIALIZED );
        virtual ~DurationDistribution();

        virtual void Configure( JsonConfigurable* pParent, const Configuration * inputJson );
        virtual void CheckConfiguration();

        void SetTypeNameDesc( const char* pTypeName, const char* pTypeDesc );
        void AddSupportedType( DistributionFunction::Enum supportedType,
                               const char* pParamName_1, const char* pParamDesc_1,
                               const char* pParamName_2, const char* pParamDesc_2 );

        virtual float CalculateDuration();

        DistributionFunction::Enum GetType() const;
        float GetParam1() const;
        float GetParam2() const;

        static void serialize( IArchive& ar, DurationDistribution& dd );

    protected:
        bool IncludeParameters( DistributionFunction::Enum typeToInclude );

    private:
        struct ParameterInfo
        {
            std::string m_ParamName_1;
            std::string m_ParamDesc_1;
            std::string m_ParamName_2;
            std::string m_ParamDesc_2;
        };
        static void serialize_map( IArchive& ar, std::map< DistributionFunction::Enum,ParameterInfo>& map );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        DistributionFunction::Enum m_Type;
        float m_Param1;
        float m_Param2;
        std::string m_TypeName;
        std::string m_TypeDesc;
        std::map<DistributionFunction::Enum,ParameterInfo> m_SupportedParameterInfoMap;
#pragma warning( pop )

    };
}