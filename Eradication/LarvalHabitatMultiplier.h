/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <map>

#include "Configure.h"
#include "VectorEnums.h"
#include "IdmApi.h"
#include "JsonConfigurableCollection.h"
#include "ExternalNodeId.h"

namespace Kernel
{
    class JsonObjectDemog;

    class LarvalHabitatMultiplierSpec : public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(LarvalHabitatMultiplierSpec)

    public:
        LarvalHabitatMultiplierSpec();
        virtual bool Configure(const Configuration* config);
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        float GetFactor() const;
        VectorHabitatType::Enum GetHabitat() const;
        std::string GetSpecies() const;

    private:
        float m_factor;
        VectorHabitatType::Enum m_habitat_name;
        std::string m_species;
    };

    class LHMSpecList : public JsonConfigurableCollection<LarvalHabitatMultiplierSpec>
    {
    public:
        LHMSpecList();
        virtual ~LHMSpecList();

    protected:
        virtual LarvalHabitatMultiplierSpec* CreateObject() override;
    };

    class IDMAPI LarvalHabitatMultiplier : public JsonConfigurable
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

    public:
        LarvalHabitatMultiplier( bool usedByIntervention = false, float minValue = 0.0f, float maxValue = FLT_MAX, float defaultValue = 1.0f );
        ~LarvalHabitatMultiplier();

        void Initialize();

        // ------------------------------------
        // --- JsonConfigurable
        // ------------------------------------
        virtual bool Configure(const Configuration * config);

        bool WasInitialized() const;
        float GetMultiplier( VectorHabitatType::Enum, const std::string& species ) const;
        void SetMultiplier( VectorHabitatType::Enum, float multiplier );
        void SetAsReduction( const LarvalHabitatMultiplier& rRegularLHM );
        void SetExternalNodeId(ExternalNodeId_t externalNodeId);

    private:
        void ProcessMultipliers(LHMSpecList &spec_list);
        bool EntryAffectsHabitatAndSpecies(LarvalHabitatMultiplierSpec * entry, 
                                           VectorHabitatType::Enum habitat_type,
                                           const std::string & species_name);
        void UnsetAllFactors(LHMSpecList &spec_list);

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        bool m_UsedByIntervention;
        float m_MinValue;
        float m_MaxValue;
        float m_DefaultValue;
        bool m_Initialized;
        std::map<VectorHabitatType::Enum,std::map<std::string,float>> m_Multiplier;
        ExternalNodeId_t m_externalNodeId;
#pragma warning( pop )
    };
}
