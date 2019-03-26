/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "LarvalHabitatMultiplier.h"
#include "SimulationConfig.h"
#include "JsonObjectDemog.h"
#include "Log.h"
#include "VectorParameters.h"
#include "VectorSpeciesParameters.h"

SETUP_LOGGING( "LarvalHabitatMultiplier" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(LarvalHabitatMultiplierSpec)
    END_QUERY_INTERFACE_BODY(LarvalHabitatMultiplierSpec)

    LarvalHabitatMultiplier::LarvalHabitatMultiplier( bool usedByIntervention, float minValue, float maxValue, float defaultValue )
    : JsonConfigurable()
    , m_UsedByIntervention( usedByIntervention )
    , m_MinValue( minValue )
    , m_MaxValue( maxValue )
    , m_DefaultValue( defaultValue )
    , m_Initialized(false)
    , m_Multiplier()
    , m_externalNodeId(0)
    {
    }

    LarvalHabitatMultiplier::~LarvalHabitatMultiplier()
    {
    }

    void LarvalHabitatMultiplier::SetExternalNodeId(ExternalNodeId_t externalNodeId)
    {
        m_externalNodeId = externalNodeId;
    }

    void LarvalHabitatMultiplier::Initialize()
    {
        // if generating schema, SimulationConfig does not exist so just return
        if( JsonConfigurable::_dryrun ) return;

        m_Multiplier.clear();

        for( int i = 0 ; i < VectorHabitatType::pairs::count(); ++i )
        {
            std::map<std::string,float> species_map;
            VectorHabitatType::Enum vht = VectorHabitatType::Enum( VectorHabitatType::pairs::get_values()[i] );

            for( auto& species : GET_CONFIGURABLE(SimulationConfig)->vector_params->vector_species_names )
            {
                // Set the value to -1 first, which we will overwrite with a specified or default value later
                species_map.insert( std::make_pair( species, m_DefaultValue) );
            }

            m_Multiplier.insert( std::make_pair( vht, species_map ) );
        }
        
        m_Initialized = true;
    }

    bool LarvalHabitatMultiplier::WasInitialized() const
    {
        return m_Initialized;
    }

    float LarvalHabitatMultiplier::GetMultiplier( VectorHabitatType::Enum vht, const std::string& species ) const
    {
        return m_Multiplier.at( vht ).at( species );
    }

    void LarvalHabitatMultiplier::SetMultiplier( VectorHabitatType::Enum vht, float multiplier )
    {
        std::map<std::string,float>& species_map = m_Multiplier[ vht ];
        for( auto& species_entry : species_map )
        {
            species_entry.second = multiplier;
        }
    }

    void LarvalHabitatMultiplier::SetAsReduction( const LarvalHabitatMultiplier& rRegularLHM )
    {
        release_assert( this->m_Multiplier.size() == rRegularLHM.m_Multiplier.size() );
        for( auto& vht_entry : m_Multiplier )
        {
            release_assert( vht_entry.second.size() == rRegularLHM.m_Multiplier.at(vht_entry.first).size() );
            std::map<std::string,float>& species_map = vht_entry.second;
            for( auto& species_entry : species_map )
            {
                // ---------------------------------------------------------------------------------
                // --- If we are calling this method, then "this" object must be used for reduction,
                // --- so we convert the values from the standard multiplier
                // ---------------------------------------------------------------------------------
                species_entry.second = 1.0 - rRegularLHM.m_Multiplier.at(vht_entry.first).at(species_entry.first);
            }
        }
    }

    bool LarvalHabitatMultiplier::EntryAffectsHabitatAndSpecies(LarvalHabitatMultiplierSpec* entry, VectorHabitatType::Enum habitat_type, const std::string& species_name)
    {
        VectorHabitatType::Enum spec_habitat = entry->GetHabitat();
        std::string spec_species = entry->GetSpecies();

        return ((spec_species.compare(species_name) == 0) || (spec_species.compare("ALL_SPECIES") == 0)) &&
            (spec_habitat == habitat_type);
    }

    void LarvalHabitatMultiplier::UnsetAllFactors(LHMSpecList &spec_list)
    {
        // For each habitat type...
        for (auto& vht_entry : m_Multiplier)
        {
            const std::string habitat_name = VectorHabitatType::pairs::lookup_key(vht_entry.first);
            std::map<std::string, float>& species_map = vht_entry.second;

            // For each species...
            for (auto& species_entry : species_map)
            {
                // Un-set the factor, to be overwritten later
                species_entry.second = -1.0f;
            }
        }
    }

    void LarvalHabitatMultiplier::ProcessMultipliers(LHMSpecList &spec_list)
    {
        // Un-set all factors (set to -1) so that we can determine whether they've been set or not
        UnsetAllFactors(spec_list);

        // For each habitat type...
        for (auto& vht_entry : m_Multiplier)
        {
            const std::string habitat_name = VectorHabitatType::pairs::lookup_key(vht_entry.first);
            std::map<std::string, float>& species_map = vht_entry.second;

            // For each species...
            for (auto& species_entry : species_map)
            {
                const std::string& species_name = species_entry.first;

                // For each configured LarvalHabitatMultiplier entry...
                for (int i = 0; i < spec_list.Size(); i++)
                {
                    // If the entry affects this habitat and species...
                    if (EntryAffectsHabitatAndSpecies(spec_list[i], vht_entry.first, species_name))
                    {
                        // Throw an error if it's already been set by an earlier entry
                        if (species_entry.second != -1.0f)
                        {
                            std::stringstream ss;
                            ss << "LarvalHabitatMultiplier over-specified for habitat and species: " << habitat_name << ", " << species_name << ". ";
                            ss << "Previously set value was " << species_entry.second << ", new value would be " << spec_list[i]->GetFactor() << "." << endl;
                            if (m_externalNodeId != 0) ss << "Affected node is " << m_externalNodeId << endl;

                            throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str());
                        }

                        // Otherwise, set the value
                        species_entry.second = spec_list[i]->GetFactor();
                        if (m_externalNodeId != 0) LOG_INFO_F("Node ID=%u with LarvalHabitatMultiplier(%s)(%s)=%0.2f\n", m_externalNodeId, habitat_name.c_str(), species_name.c_str(), species_entry.second);
                    }
                }

                // If the habitat/species remains unset, set it to the default
                if (species_entry.second == -1.0f)
                {
                    species_entry.second = m_DefaultValue;
                }
            }
        }

        return;
    }

    bool LarvalHabitatMultiplier::Configure(const Configuration * config)
    {
        if ( !m_Initialized && !JsonConfigurable::_dryrun )
        {
            throw InitializationException(__FILE__, __LINE__, __FUNCTION__, "Configuring un-initialized LarvalHabitatMultiplier");
        }

        LHMSpecList habitat_species_specs;
        initConfigComplexType("LarvalHabitatMultiplier", &habitat_species_specs, LarvalHabitatMultiplier_DESC_TEXT);

        bool result = false;

        try
        {
            result = JsonConfigurable::Configure(config);

            if (!JsonConfigurable::_dryrun && result)
            {
                ProcessMultipliers(habitat_species_specs);
            }
        }
        catch (DetailedException& e)
        {
            std::stringstream ss;
            ss << e.GetMsg();
            if (m_externalNodeId != 0) ss << endl << "LarvalHabitatMultiplier specified for node " << m_externalNodeId;
            throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, ss.str().c_str());
        }

        return result;
    }

    LarvalHabitatMultiplierSpec::LarvalHabitatMultiplierSpec()
        : JsonConfigurable()
        , m_factor(1.0f)
        , m_habitat_name(VectorHabitatType::Enum::ALL_HABITATS)
        , m_species("ALL_SPECIES")
    {
    }

    bool LarvalHabitatMultiplierSpec::Configure(const Configuration* config)
    {
        initConfig("Habitat", m_habitat_name, config, MetadataDescriptor::Enum("Habitat", LHMSpec_Habitat_DESC_TEXT, MDD_ENUM_ARGS(VectorHabitatType)));
        initConfigTypeMap("Factor", &m_factor, LHMSpec_Factor_DESC_TEXT, 0, FLT_MAX, 1.0f);
        initConfigTypeMap("Species", &m_species, LHMSpec_Species_DESC_TEXT);

        return JsonConfigurable::Configure(config);
    }

    float LarvalHabitatMultiplierSpec::GetFactor() const 
    { 
        return m_factor; 
    }

    VectorHabitatType::Enum LarvalHabitatMultiplierSpec::GetHabitat() const 
    { 
        return m_habitat_name;
    }

    std::string LarvalHabitatMultiplierSpec::GetSpecies() const 
    { 
        return m_species; 
    }
    
    LarvalHabitatMultiplierSpec* LHMSpecList::CreateObject()
    {
        return new LarvalHabitatMultiplierSpec();
    }

    LHMSpecList::LHMSpecList()
        : JsonConfigurableCollection("LHMSpecList")
    {
    }

    LHMSpecList::~LHMSpecList()
    {
    }
}
