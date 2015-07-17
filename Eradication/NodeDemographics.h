/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "IdmApi.h"
#include <string>
#include <set>
#include <map>
#include "BoostLibWrapper.h"

#include "suids.hpp"
#include "Contexts.h"
#include "Configure.h"
#include "JsonObjectDemog.h"

class Configuration;

namespace Kernel
{
    struct NodeDemographics;
    class NodeDemographicsFactory;

    struct IDMAPI DemographicsContext
    {
        static bool using_compiled_demog;
    private:
        std::map<std::string, std::string> * stringTable;

        friend class Kernel::NodeDemographicsFactory;
        friend struct Kernel::NodeDemographics;

#if USE_BOOST_SERIALIZATION
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, DemographicsContext& nd, const unsigned int /* file_version */); // { ar & stringTable; }
#endif

#if USE_JSON_SERIALIZATION
    public:

        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };

    struct IDMAPI NodeDemographics
    {
    public:
        virtual ~NodeDemographics();

        virtual const NodeDemographics operator[](const std::string& key) const;
        const NodeDemographics operator[](int index) const;
        size_t size() const;
        bool Contains(const std::string& key) const;

        double   AsDouble() const ;
        int      AsInt()    const ;
        bool     AsBool()   const ;
        uint64_t AsUint64() const ;
        string   AsString() const ;

        void SetContext(const DemographicsContext *context, INodeContext *parent);
        uint32_t GetNodeID() const { return nodeID; };

        // Return the key/string used in the demographics file for this value
        std::string GetJsonKey() const { return valueKey; } // !!! See comment on get_array() !!!

        // These operators were initially created for testing.
        bool operator==( const NodeDemographics& rThat ) const ;
        bool operator!=( const NodeDemographics& rThat ) const ;

        // Explicit checks for required keys for Individual Properties transitions
        void validateIPTransition() const;

        std::string ToString() const { return jsonValue.ToString(); }
        bool IsObject() const { return jsonValue.IsObject(); }

    protected:
        friend class NodeDemographicsFactory;
        friend class Node;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! Do not return a const reference because one could write code like
        // !!!     const JsonObjectDemog& rArray = demo["name"].get_array();
        // !!! the operator[ string ] will create a new NodeDemographics object,
        // !!! but that object is on the stack and is only used to call get_array().
        // !!! It is deleted when it goes to the next line.  Returning a reference
        // !!! would be to something that does not exist on the stack.
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        JsonObjectDemog get_array() const;

        NodeDemographics() : jsonValue(), string_table(nullptr), parent(nullptr), nodeID(0), valueKey(), parentKey() { }
        NodeDemographics( const JsonObjectDemog& v,
                          std::map<std::string, std::string> *s, 
                          INodeContext *p, 
                          uint32_t nodeid,
                          const std::string& rValueKey,
                          const std::string& rParentKey )
            : jsonValue(v)
            , string_table(s)
            , parent(p)
            , nodeID(nodeid)
            , valueKey( rValueKey )
            , parentKey( rParentKey )
        {
        }
        NodeDemographics( const NodeDemographics& rND )
            : jsonValue(rND.jsonValue)
            , string_table(rND.string_table)
            , parent(rND.parent)
            , nodeID(rND.nodeID)
            , valueKey(rND.valueKey )
            , parentKey( rND.parentKey )
        {
        }

        static std::string getMissingParamHelperMessage(const std::string &missing_param);

        std::string GetFailedToInterpretMessage( const char* pExpType ) const ;

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        JsonObjectDemog jsonValue;
        std::map<std::string,std::string> * string_table;
        INodeContext * parent;
        uint32_t nodeID ; // external ID of the node
        std::string valueKey ; // The key/string used in the demographics file for this value/object
        std::string parentKey ; // The key/string used in the demographics file for the parent i.e. demographics[ parentKey ][ valueKey ]
#pragma warning( pop )

        friend struct NodeDemographicsDistribution;

#if USE_JSON_SERIALIZATION
    public:

        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
        friend class ::boost::serialization::access;

        template<class Archive>
        friend void serialize(Archive & ar, NodeDemographics &nd, const unsigned int /* file_version */);
#endif
    };

    class IDMAPI NodeDemographicsFactory : public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(NodeDemographicsFactory)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static const uint32_t default_geography_torus_size;

        static NodeDemographicsFactory* CreateNodeDemographicsFactory(boost::bimap<uint32_t, suids::suid> * nodeid_suid_map, const ::Configuration *config);
        ~NodeDemographicsFactory();

        DemographicsContext* CreateDemographicsContext();
        NodeDemographics* CreateNodeDemographics(INodeContext *parent_node);

        // This was initially created for testing so that one can create
        // a node demographics on the fly.
        static NodeDemographics* CreateNodeDemographics( JsonObjectDemog pValue,
                                                         std::map<std::string, std::string> *pStringTable, 
                                                         INodeContext *pParentNode, 
                                                         uint32_t nodeid,
                                                         const std::string& rValueKey,
                                                         const std::string& rParentKey );

        const std::vector<uint32_t>& GetNodeIDs() { return nodeIDs; }
        const std::string GetIdReference() { return idreference; }

        // If the user selected to use the default demographics, this routine can be used
        // to write the demographics to a file once the demographics have been initialized.
        void WriteDefaultDemographicsFile( const std::string& rFilename );

        static std::vector<std::string> ConvertLegacyStringToSet(const std::string& legacyString);

        static void SetDemographicsFileList( const std::vector<std::string>& rFilenames )
        {
            demographics_filenames_list = rFilenames;
        };

        // Put this here so it can be used by unit tests.  this returns a copy of the string
        // because in release from the unit tests it would get a null pointer exception.
        // Probably something to do with going across the DLL boundary and the warning 4251 warning message
        static std::vector<std::string> GetDemographicsFileList() { return demographics_filenames_list; };

    private:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        static const std::string default_node_demographics_str;

        boost::bimap<uint32_t, suids::suid> * nodeid_suid_map;
        std::vector<uint32_t> nodeIDs; // sucks to have to keep an extra copy of these; I guess we could expose through an iterator, but for now, I prefer this...
        std::string idreference;

        std::map<std::string, std::string> * full_string_table;

        // per-layer data
        std::vector<std::string> demographics_filenames;
        std::vector<JsonObjectDemog> layer_defaults;
        std::vector<std::map<std::string, std::string>> layer_string_sub_tables;
        std::vector<std::map<std::string, std::string>> layer_string_value2key_tables;

        // node-id to raw data from demographics file for each layer
        // NOTE: we don't want to use unordered_map here because we want the order maintained.
        std::vector<std::map<uint32_t,JsonObjectDemog>> nodedata_maps ;

        static std::vector<std::string> demographics_filenames_list;

#pragma warning( pop )

        NodeDemographicsFactory() : nodeid_suid_map(), nodeIDs(), idreference(), full_string_table( nullptr ), demographics_filenames(), layer_defaults(), layer_string_sub_tables(), layer_string_value2key_tables(), nodedata_maps() { };
        NodeDemographicsFactory(boost::bimap<uint32_t, suids::suid> * nodeid_suid_map)
            : nodeid_suid_map( nodeid_suid_map )
            , nodeIDs()
            , idreference()
            , full_string_table( nullptr )
            , demographics_filenames()
            , layer_defaults()
            , layer_string_sub_tables()
            , layer_string_value2key_tables()
            , nodedata_maps()
        { 
        };
        void Initialize(const ::Configuration *config);

        virtual bool Configure( const Configuration* config );

        // Create json object of the data that is unique for this default node.
        JsonObjectDemog CreateDefaultNodeDemograhics( uint32_t nodeid );

        std::vector<std::string> GetDemographicFileNames(const ::Configuration* config);

        // Set the IdReference for the demographics
        void SetIdReference( int layer, const std::string& rDemoFileName, const JsonObjectDemog& rMetadata );

        // Extract the property strings from a json data object (i.e. the object from the file).
        // This creates a string table from the data object and then populates the tables.
        void UpdateStringTablesFromData( int layer, 
                                         const JsonObjectDemog& rData,
                                         std::string* pLastStringValue, 
                                         std::set<std::string>* pStringTableValues );

        // Update the full_string_table or the layer_string_sub_tables with the given json table of strings
        void UpdateStringTables( int layer, 
                                 const JsonObjectDemog& rStringtable,
                                 std::string* pLastStringValue, 
                                 std::set<std::string>* pStringTableValues );

        // Extract the node data from the given file.
        // This will also update the string tables as necessary.
        bool ReadNodeData( bool isUnCompiled,
                           int layer, 
                           const JsonObjectDemog& rData,
                           const std::string& rDemographicsFilename,
                           std::string* pLastStringValue, 
                           std::set<std::string>* pStringTableValues );

        std::string GetNextStringValue(std::string last_value, std::set<std::string> used_values );
        void TranslateNodeData( const JsonObjectDemog& val, int layer, JsonObjectDemog& existing_val );

        void addToStringTable( const JsonObjectDemog& rNodedata, JsonObjectDemog* pStringTable );
        void addToStringTable( const JsonObjectDemog& val, std::map<std::string, std::string> * string_table );
    };

    struct IDMAPI NodeDemographicsDistribution : public NodeDemographics
    {
    public:
        // This method was initially created for for testing so that one could create
        // a distribution with known values.
        static NodeDemographicsDistribution* CreateDistribution( const NodeDemographics &nd,
                                                                 int numAxes, 
                                                                 std::vector<int> &rNumPopGroups,
                                                                 std::vector< std::vector<double> > &rPopGroups, 
                                                                 std::vector<double> &rResultValues, 
                                                                 std::vector< std::vector<double> > &rDistValues) ;


        static NodeDemographicsDistribution* CreateDistribution( const NodeDemographics& demographics );
        static NodeDemographicsDistribution* CreateDistribution( const NodeDemographics& demographics, const std::string& rAxis1 );
        static NodeDemographicsDistribution* CreateDistribution( const NodeDemographics& demographics, const std::string& rAxis1, const std::string& rAxis2 );
        static NodeDemographicsDistribution* CreateDistribution( const NodeDemographics& demographics, const std::string& rAxis1, const std::string& rAxis2, const std::string& rAxis3 );
        static NodeDemographicsDistribution* CreateDistribution( const NodeDemographics& demographics, const std::vector<std::string>& rAxisNames );
        virtual ~NodeDemographicsDistribution() { }

        float DrawFromDistribution(float randdraw, ...) const;
        float DrawResultValue(double axis0value, ...) const;

        // These operators were initially created for testing.
        bool operator==( const NodeDemographicsDistribution& rThat ) const ;
        bool operator!=( const NodeDemographicsDistribution& rThat ) const ;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        // A step towards limiting the lookup in Node for demographic_distributions by arbitrary keys
        // -- General --
        static const std::string FertilityDistribution;
        static const std::string MortalityDistribution;
        static const std::string MortalityDistributionMale;
        static const std::string MortalityDistributionFemale;
        static const std::string AgeDistribution;

        static const std::string HIVCoinfectionDistribution;
        static const std::string HIVMortalityDistribution;

        // -- Polio --
        static const std::string maternal_antibody_distribution1;
        static const std::string maternal_antibody_distribution2;
        static const std::string maternal_antibody_distribution3;
        static const std::string mucosal_memory_distribution1;
        static const std::string mucosal_memory_distribution2;
        static const std::string mucosal_memory_distribution3;
        static const std::string humoral_memory_distribution1;
        static const std::string humoral_memory_distribution2;
        static const std::string humoral_memory_distribution3;
        static const std::string fake_time_since_last_infection_distribution;

        // -- Malaria --
        static const std::string MSP_mean_antibody_distribution;
        static const std::string nonspec_mean_antibody_distribution;
        static const std::string PfEMP1_mean_antibody_distribution;
        static const std::string MSP_variance_antibody_distribution;
        static const std::string nonspec_variance_antibody_distribution;
        static const std::string PfEMP1_variance_antibody_distribution;
#pragma warning( pop )

        // -----------------------------
        // --- Methods only for testing
        // -----------------------------
        const std::vector< std::vector<double> >& GetPopGroups() const { return pop_groups; }

    private:
        friend struct NodeDemographics;
        friend class Node;

        // Create a string with the names comma delimited.
        static std::string CreateListOfNames( const JsonObjectDemog& rAxisNames ) ;
        static std::string CreateListOfNames( const std::vector<std::string>& rAxisNames );

        // Check that the given rJsonArray has the dimensions that is expected.
        // This is used to verify that ResultValues and DistributionValues have the
        // the correct number of dimensions.
        static void CheckArraySize( bool isCheckingDistributionValues,
                                    const std::string& rDistName, 
                                    uint32_t nodeID, 
                                    const std::string& rArrayName,
                                    const std::vector<int>& rNumPopGroups, 
                                    int depth,
                                    int iThElement,
                                    int jThElement,
                                    const JsonObjectDemog& rJsonArray );


        NodeDemographicsDistribution() { } // default constructor for serialization of demographic_distributions by Node
        NodeDemographicsDistribution( const NodeDemographics &nd,
                                      int _num_axes, 
                                      std::vector<int> &_num_pop_groups,
                                      std::vector< std::vector<double> > &_pop_groups, 
                                      std::vector<double> &_result_values, 
                                      std::vector< std::vector<double> > &_dist_values )
                                : NodeDemographics( nd )
                                , num_axes(_num_axes)
                                , num_pop_groups(_num_pop_groups)
                                , pop_groups(_pop_groups)
                                , result_values(_result_values)
                                , dist_values(_dist_values) { }

        float pickDistributionValue(std::vector<float> &groupvals, float randdraw = 0, int start_axis = 0, int dim_span = 1, int slot = 0) const;

        static void applyResultScaleFactor( const JsonObjectDemog& _result_values_orig, std::vector<double> &_result_values, double scale_factor, int dim_span, int slot);
        static void flattenDist( const JsonObjectDemog& _dist_values_orig, std::vector< std::vector<double> > &_dist_values, int dim_span, int slot);

        int num_axes;

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::vector<int> num_pop_groups;
        std::vector< std::vector<double> > pop_groups;

        std::vector<double> result_values;
        std::vector< std::vector<double> > dist_values;
#pragma warning( pop )

#if USE_JSON_SERIALIZATION
    public:

        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, NodeDemographicsDistribution &ndd, const unsigned int /* file_version */);
#endif
    };
}
