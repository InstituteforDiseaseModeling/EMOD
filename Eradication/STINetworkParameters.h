/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "Configure.h"
#include "HIVEnums.h"
#include "IRelationship.h"
#include "SimulationEnums.h"        // For Gender

namespace Kernel
{

    class STINetworkParameters : public JsonConfigurable
    {
        friend class SimulationConfig;
        friend class IndividualHumanSTI;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        explicit STINetworkParameters( const std::string& key );

        static STINetworkParameters* CreateSTINetworkParameters( const std::string& key, const Configuration* pConfig );
        virtual ~STINetworkParameters();
        virtual bool Configure( const ::Configuration *json );
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

    protected:

        std::string key_colon_val;

        ExtraRelationalFlagType::Enum extra_relational_flag_type;
        float prob_extra_relational[RelationshipType::COUNT][Gender::COUNT];
        float max_simultaneous_rels[RelationshipType::COUNT][Gender::COUNT];
    };

    struct IIndividualHuman ;

    class STINetworkParametersMap : public JsonConfigurable
    {
        friend class ::boost::serialization::access;
    public:
        STINetworkParametersMap();
        ~STINetworkParametersMap();

        // -----------------------------
        // --- JsonConfigurable Methods
        // -----------------------------
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

        virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
        virtual json::QuickBuilder GetSchema();

        // -----------------
        // --- Other Methods
        // -----------------
        const STINetworkParameters* GetParameters( IIndividualHuman* pIndividual, 
                                                   const char *prop, 
                                                   const char* new_value );

    protected:
        bool HasIndividualPropertyParams() ;

        const STINetworkParameters* GetFirst() ;
        const STINetworkParameters* Find( const std::string& rPropertyName, 
                                            const std::string& rPropertyValue );
        void ValidateProperties();

    private:
        bool has_ip_params ;
        bool have_properties_been_validated ;
        std::string property_name ;
        std::map< std::string, STINetworkParameters * > param_map ;
    };
}
