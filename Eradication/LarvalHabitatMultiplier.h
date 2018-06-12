/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>

#include "Configure.h"
#include "VectorEnums.h"
#include "IdmApi.h"

namespace Kernel
{
    class JsonObjectDemog;

    class IDMAPI LarvalHabitatMultiplier : public JsonConfigurable, public IComplexJsonConfigurable
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

    public:
        LarvalHabitatMultiplier( bool usedByIntervention = false, float minValue = 0.0f, float maxValue = FLT_MAX, float defaultValue = 1.0f );
        ~LarvalHabitatMultiplier();

        void Initialize();

        // ------------------------------------
        // --- IComplexJsonConfigurable Methods
        // ------------------------------------
        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
        virtual json::QuickBuilder GetSchema() override;
        virtual bool  HasValidDefault() const override { return false; }

        void Read( const JsonObjectDemog& rJsonData, uint32_t externalNodeId );
        bool WasInitialized() const;
        float GetMultiplier( VectorHabitatType::Enum, const std::string& species ) const;
        void SetMultiplier( VectorHabitatType::Enum, float multiplier );
        void SetAsReduction( const LarvalHabitatMultiplier& rRegularLHM );

    private:
        void CheckRange( float multipler, 
                         uint32_t externalNodeId, 
                         const std::string& rHabitatName, 
                         const std::string& rSpeciesName );
        void CheckIfConfigured( VectorHabitatType::Enum habitatType );
        void CheckIfConfigured( VectorHabitatType::Enum havitatType, const std::string& species );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        bool m_UsedByIntervention;
        float m_MinValue;
        float m_MaxValue;
        float m_DefaultValue;
        bool m_Initialized;
        std::map<VectorHabitatType::Enum,std::map<std::string,float>> m_Multiplier;
#pragma warning( pop )
    };
}
