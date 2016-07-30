/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <ctime>

#include "Debug.h"
#include "NodeDemographics.h"
#include "Environment.h"
#include "FileSystem.h"
#include "Exceptions.h"
#include "IdmString.h"
#include "Log.h"

#define EPSILON (FLT_EPSILON)

using namespace std;

typedef std::map< std::string, std::string > tMapType;

static const char * _module = "NodeDemographics";

namespace Kernel
{

GET_SCHEMA_STATIC_WRAPPER_IMPL(NodeDemographics,NodeDemographicsFactory)
BEGIN_QUERY_INTERFACE_BODY(NodeDemographicsFactory)
END_QUERY_INTERFACE_BODY(NodeDemographicsFactory)

std::vector<std::string> NodeDemographicsFactory::demographics_filenames_list;
bool DemographicsContext::using_compiled_demog = true;

const std::string NodeDemographicsFactory::default_node_demographics_str = string(
"{ \n\
    \"NodeID\": -1, \n\
    \"NodeAttributes\": { \n\
        \"Latitude\": -1, \n\
        \"Longitude\": -1, \n\
        \"Altitude\": 0, \n\
        \"Airport\": 0, \n\
        \"Region\": 0, \n\
        \"Seaport\": 0, \n\
        \"BirthRate\": 0.00008715, \n\
        \"Urban\": 0, \n\
        \"AbovePoverty\": 0.5 \n\
    }, \n\
    \"IndividualAttributes\": { \n\
        \"AgeDistributionFlag\": 3, \n\
        \"AgeDistribution1\": 0.000118, \n\
        \"AgeDistribution2\": 0, \n\
        \"PrevalenceDistributionFlag\": 0, \n\
        \"PrevalenceDistribution1\": 0.0, \n\
        \"PrevalenceDistribution2\": 0, \n\
        \"ImmunityDistributionFlag\": 0, \n\
        \"ImmunityDistribution1\": 0, \n\
        \"ImmunityDistribution2\": 0, \n\
        \"RiskDistributionFlag\": 0, \n\
        \"RiskDistribution1\": 0, \n\
        \"RiskDistribution2\": 0, \n\
        \"MigrationHeterogeneityDistributionFlag\": 0, \n\
        \"MigrationHeterogeneityDistribution1\": 1, \n\
        \"MigrationHeterogeneityDistribution2\": 0 \n\
    } \n\
} ");

const NodeDemographics NodeDemographics::operator[]( const string& key ) const
{
    if(string_table->count(key) != 0 || DemographicsContext::using_compiled_demog == false)
    {
        try
        {
            JsonObjectDemog v ;
            if( jsonValue.Contains( (*string_table)[key].c_str() ) )
            {
                v = jsonValue[ (*string_table)[key].c_str() ] ;
            }
            else
            {
                // Ideally we'd use 0 here, not 1, but for unknown reason, string table not always 
                // completely empty when it should be.
                if( (*string_table).size() <= 1 )
                {
                    // -----------------------------------------------------------------------------
                    // --- DMB 8-14-2014  This statement/feature is older than when I added support
                    // --- for uncompiled demographic files. As of 8-14-2014, the uncompiled suport
                    // --- uses the string table so that we can support multiple demographics files
                    // --- where some are compiled and some not.
                    // -----------------------------------------------------------------------------
                    // empty string table means we're using uncompiled NodeDemographics
                    (*string_table).erase( key );
                    if( jsonValue.Contains( key.c_str() ) )
                    {
                        v = jsonValue[key.c_str()];
                    }
                }
            }
            if( v.IsNull() )
            {
                std::stringstream msg ;
                msg << "NodeID " << nodeID << "'s '"<< valueKey <<"' object doesn't contain expected demographics attribute '" + key + "'." ;
                throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.str().c_str() );
            }

            return NodeDemographics(v, string_table, parent, nodeID, key, valueKey);
        }
        catch( NodeDemographicsFormatErrorException& )
        {
            // rethrow this because we are expecting to throw it, but others need to be caught.
            throw ;
        }
        catch( std::runtime_error& e )
        { 
            std::stringstream msg ;
            msg << "Unexpected error occurred getting attribute '" << key << "' from parent object '" << valueKey << "' in the demographics for nodeID=" << nodeID << ". Error=" << e.what() ;
            throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.str().c_str() );
        }
    }
    else
    {
        string suggestion = getMissingParamHelperMessage(key);
        std::stringstream msg ;
        msg << "Demographics attribute '" << key << "' not present for any node in any demographics layer for NodeID=" << nodeID << ".\n" + suggestion ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.str().c_str() );
    }
}

const NodeDemographics NodeDemographics::operator[]( int index ) const
{
    if( !jsonValue.IsArray() )
    {
        std::string msg = GetFailedToInterpretMessage( "array" ) ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.c_str() );
    }
    return NodeDemographics( jsonValue[IndexType(index)], string_table, parent, nodeID, "array", valueKey );
}

std::string NodeDemographics::GetFailedToInterpretMessage( const char* pExpType ) const
{
    std::stringstream msg ;
    msg << "Failed to interpret value on the demographics attribute " ;
    if( parentKey.empty() )
    {
        msg << "'" << valueKey << "'" ;
    }
    else
    {
        msg << "[ '" << parentKey << "' ][ '" << valueKey << "' ]" ;
    }
    msg << " for nodeID=" << nodeID << ".  The attribute is supposed to be a '"<< pExpType << "' but is a '" << jsonValue.GetTypeName() <<"'." ;
    return msg.str() ;
}

JsonObjectDemog NodeDemographics::get_array() const
{
    if( !jsonValue.IsArray() )
    {
        std::string msg = GetFailedToInterpretMessage( "array" ) ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.c_str() );
    }
    return jsonValue;
}


double NodeDemographics::AsDouble() const
{
    double db = 0.0 ;
    try
    {
        db = jsonValue.AsDouble();
    }
    catch( SerializationException& )
    {
        std::string msg = GetFailedToInterpretMessage( "double" ) ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.c_str() );
    }
    return db ;
}

int NodeDemographics::AsInt() const
{
    int integer = 0 ;
    try
    {
        integer = jsonValue.AsInt();
    }
    catch( SerializationException& )
    {
        std::string msg = GetFailedToInterpretMessage( "int" ) ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.c_str() );
    }
    return integer ;
}

bool NodeDemographics::AsBool() const
{
    bool flag = false;
    try
    {
        flag = jsonValue.AsBool();
    }
    catch( SerializationException& )
    {
        std::string msg = GetFailedToInterpretMessage( "bool" ) ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.c_str() );
    }
    return flag ;
}

uint64_t NodeDemographics::AsUint64() const
{
    uint64_t integer = 0 ;
    try
    {
        integer = jsonValue.AsUint64();
    }
    catch( SerializationException& )
    {
        std::string msg = GetFailedToInterpretMessage( "uint64" ) ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.c_str() );
    }
    return integer ;
}

string NodeDemographics::AsString() const
{
    std::string ret_str ;
    try
    {
        ret_str = jsonValue.AsString();
    }
    catch( SerializationException& )
    {
        std::string msg = GetFailedToInterpretMessage( "String" ) ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.c_str() );
    }
    return ret_str ;
}

size_t NodeDemographics::size() const
{
    if( !jsonValue.IsArray() )
    {
        std::string msg = GetFailedToInterpretMessage( "array (size)" ) ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.c_str() );
    }

    return jsonValue.size();
}

bool NodeDemographics::Contains(const string& key) const
{
    if(string_table->count(key) != 0)
    {
        if( (*string_table).find( key ) != (*string_table).end() )
        {
            return jsonValue.Contains( (*string_table)[key].c_str() );
        }
        else
        {
            return false;
        }
    }
    else if( (*string_table).size() == 0 )
    {
        return jsonValue.Contains( key.c_str() );
    }
    return false;
}

void NodeDemographics::SetContext(const DemographicsContext *context, INodeContext *parent)
{
    this->parent = parent;
    string_table = context->stringTable;
}

std::string NodeDemographics::getMissingParamHelperMessage(const std::string &missing_param)
{
    stringstream ss;
    ss << "Was ";

    if(missing_param == "AgeDistribution")               ss << "EnableAgeInitializationDistribution set to 1";
    else if(missing_param == "FertilityDistribution")    ss << "Birth_Rate_Dependence set to INDIVIDUAL_PREGNANCIES_BY_URBAN_AND_AGE or INDIVIDUAL_PREGNANCIES_BY_AGE_AND_YEAR";
    else if(missing_param == "MortalityDistribution")    ss << "Death_Rate_Dependence set to NONDISEASE_MORTALITY_BY_AGE_AND_GENDER";
    else if(missing_param == "MortalityDistributionMale")    ss << "Death_Rate_Dependence set to NONDISEASE_MORTALITY_BY_YEAR_AND_AGE_FOR_EACH_GENDER";
    else if(missing_param == "MortalityDistributionFemale")    ss << "Death_Rate_Dependence set to NONDISEASE_MORTALITY_BY_YEAR_AND_AGE_FOR_EACH_GENDER";
    else if(missing_param == "mucosal_memory_distribution1")     ss << "EnableImmunityInitializationDistribution set to 1";
    else if(missing_param == "mucosal_memory_distribution2")     ss << "EnableImmunityInitializationDistribution set to 1";
    else if(missing_param == "mucosal_memory_distribution3")     ss << "EnableImmunityInitializationDistribution set to 1";
    else if(missing_param == "humoral_memory_distribution1")     ss << "EnableImmunityInitializationDistribution set to 1";
    else if(missing_param == "humoral_memory_distribution2")     ss << "EnableImmunityInitializationDistribution set to 1";
    else if(missing_param == "humoral_memory_distribution3")     ss << "EnableImmunityInitializationDistribution set to 1";
    else if(missing_param == "time_since_last_infection_distribution")     ss << "EnableImmunityInitializationDistribution set to 1";
    else   ss << "some config.json parameter changed";

    ss << " without the demographics layer(s) specified containing the necessary parameters?";

    return ss.str();
}

NodeDemographics::~NodeDemographics()
{

}

bool NodeDemographics::operator==( const NodeDemographics& rThat ) const
{
    if( this->jsonValue   != rThat.jsonValue  ) return false ;
    if( *(this->parent)   != *(rThat.parent)  ) return false ;
    if( this->nodeID      != rThat.nodeID     ) return false ;
    if( this->valueKey    != rThat.valueKey   ) return false ;
    if( this->parentKey   != rThat.parentKey  ) return false ;

    if( this->string_table->size() != rThat.string_table->size() ) return false ;
    if( !std::equal( this->string_table->cbegin(), this->string_table->cend(), rThat.string_table->cbegin() ) ) return false ;

    return true ;
}

bool NodeDemographics::operator!=( const NodeDemographics& rThat ) const 
{
    return !(*this == rThat);
}


NodeDemographicsFactory * NodeDemographicsFactory::CreateNodeDemographicsFactory( boost::bimap<uint32_t, suids::suid> * nodeid_suid_map, 
                                                                                  const ::Configuration *config,
                                                                                  bool isDataInFiles,
                                                                                  uint32_t torusSize,
                                                                                  uint32_t defaultPopulation )
{
    NodeDemographicsFactory* factory = _new_ NodeDemographicsFactory(nodeid_suid_map);

    try
    {
        factory->Initialize( config, isDataInFiles, torusSize, defaultPopulation );
    }
    catch( std::exception& )
    {
        delete factory ;
        factory = nullptr ;
        throw ;  //rethrow the exception preserving the object
    }

    return factory;
}

std::vector<std::string> NodeDemographicsFactory::ConvertLegacyStringToSet(const std::string& legacyString)
{
    std::vector<IdmString> filenames = IdmString(legacyString).split(';');
    std::vector<std::string> vs;
    for(auto &fn : filenames)
    {
        vs.push_back(std::string(fn));
    }
    return vs;
}

bool NodeDemographicsFactory::Configure(const Configuration* config)
{
    std::string legacy_filename;
    default_node_population = 1000;
    initConfigTypeMap( "Demographics_Filename", &legacy_filename, Demographics_Filename_DESC_TEXT );
    initConfigTypeMap( "Demographics_Filenames", &demographics_filenames_list, Demographics_Filenames_DESC_TEXT );

    bool resetDefaults = JsonConfigurable::_useDefaults;
    bool resetTrackMissing = JsonConfigurable::_track_missing;
    JsonConfigurable::_useDefaults=true;
    JsonConfigurable::_track_missing=false;
    bool configured = JsonConfigurable::Configure( config );
    if( !JsonConfigurable::_dryrun && 
        (legacy_filename != JsonConfigurable::default_string) && 
        (demographics_filenames_list.size() > 0) )
    {
        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Demographics_Filename", legacy_filename.c_str(), "Demographics_Filenames", "<not empty>", "Must specify one or the other but not both." );
    }
    JsonConfigurable::_useDefaults = resetDefaults;
    JsonConfigurable::_track_missing = resetTrackMissing;

    if (demographics_filenames_list.empty() && !JsonConfigurable::_dryrun)
    {
        if (legacy_filename.compare("UNINITIALIZED")!=0)
        {
            LOG_WARN("DeprecationWarning: Specification of multiple files with \"Demographics_Filename\" parameter as semicolon-delimited string will soon be deprecated. Use the \"Demographics_Filenames\" parameter (note plural) and specify its value as a JSON array, e.g. \"Demographics_Filenames\" : [\"filename1\",\"filename2\"]\n");
            demographics_filenames_list = ConvertLegacyStringToSet(legacy_filename);
        }
        else
        {
            throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, "config.json", "Demographics_Filenames" );
        }
    }

    return configured;
}

void NodeDemographicsFactory::Initialize( const ::Configuration* config,
                                          bool isDataInFiles,
                                          uint32_t torusSize,
                                          uint32_t defaultPopulation )
{
    try
    {
        full_string_table = new std::map<string, string>();

        if( isDataInFiles )
        {
            demographics_filenames = GetDemographicFileNames( config );

            string last_string_value = "";
            set<string> string_table_values;

            for(int layer = 0; layer < demographics_filenames.size(); layer++)
            {
                string demo_filename = demographics_filenames[ layer ] ;

                // Insert entry so that UpdateStringTables() can update the entry if necessary
                std::map<string, string> string_sub_table;
                layer_string_sub_tables.push_back(string_sub_table);
                std::map<string, string> string_val2key_table;
                layer_string_value2key_tables.push_back(string_val2key_table);

                // ---------------------------------
                // --- Read and parse the json file
                // ---------------------------------
                JsonObjectDemog json ;
                try
                {
                    json.ParseFile( demo_filename.c_str() );
                }
                catch( SerializationException& rse )
                {
                    throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, demo_filename.c_str(), rse.GetMsg() );
                }

                // ----------------------------------------------
                // --- Get the data from from the Metadata object
                // ----------------------------------------------
                if( !json.Contains( "Metadata" ) )
                {
                    throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, demo_filename.c_str(), "Missing the 'Metadata' object." );
                }
                JsonObjectDemog metadata = json["Metadata"] ;

                if( !metadata.Contains( "NodeCount" ) )
                {
                    throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, demo_filename.c_str(), "Missing the 'Metadata.NodeCount' object." );
                }

                int nodecount = metadata.GetInt("NodeCount");
                if( nodecount <= 0 )
                {
                    std::stringstream msg ;
                    msg << "'NodeCount' = " << nodecount << ".  It must be positive." ;
                    throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, demo_filename.c_str(), msg.str().c_str() );
                }
                SetIdReference( layer, demo_filename, metadata );

                // ------------------------------------
                // --- Get the default data if present
                // ------------------------------------
                JsonObjectDemog nodedefaults ;
                if( json.Contains( "Defaults" ) )
                {
                    nodedefaults = json["Defaults"];
                }
                layer_defaults.push_back(nodedefaults);

                // ----------------------------------------------
                // --- Determine if the file is compiled or not.
                // ----------------------------------------------
                if( !json.Contains("StringTable") &&  json.Contains("NodeOffsets") )
                {
                    throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, demo_filename.c_str(), 
                        "Invalid compiled file.  The file contains 'NodeOffsets' but does not have 'StringTable'.");
                }
                else if( json.Contains("StringTable") && !json.Contains("NodeOffsets") )
                {
                    throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, demo_filename.c_str(), 
                        "Invalid compiled file.  The file contains 'StringTable' but does not have 'NodeOffsets'.");
                }
                bool is_uncompiled_file = !json.Contains("StringTable") && !json.Contains("NodeOffsets");

                // ---------------------------
                // --- Update the StringTable
                // ---------------------------
                if( is_uncompiled_file )
                {
                    if( !nodedefaults.IsNull() )
                    {
                        UpdateStringTablesFromData( layer, nodedefaults,  &last_string_value, &string_table_values );
                    }
                }
                else
                {
                    UpdateStringTables( layer, json["StringTable"], &last_string_value, &string_table_values );
                }

                // ----------------------------------------------------------------------
                // --- Get the data for each node and place into a map for retriel later
                // ----------------------------------------------------------------------

                bool layer_has_nodes = ReadNodeData( is_uncompiled_file,
                                                     layer, 
                                                     json,
                                                     demo_filename, 
                                                     &last_string_value, 
                                                     &string_table_values );

                // -----------------------------------------------------------------------------
                // --- If this layer did not ahve any nodes, make sure it is a valid situation
                // -----------------------------------------------------------------------------
                if( !layer_has_nodes )
                {
                    if( layer == 0 )
                    {
                        // if the base layer doesn't have nodes, there is a problem
                        std::stringstream msg ;
                        msg << "It is the base layer demographics file.  It must have nodes defined in order to be a base layer." ;
                        if( is_uncompiled_file )
                            msg << "  The file is assumed to be uncompiled." ;
                        else
                            msg << "  The file is assumed to be compiled." ;

                        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, demo_filename.c_str(), msg.str().c_str());
                    }
                }
            }
        }
        else // default geography
        {
            torus_size = torusSize;
            default_population = defaultPopulation; 
            release_assert( torus_size >= 3 );

            std::stringstream ss;
            ss << "default " << torus_size << "x" << torus_size << " torus geography";
            idreference = ss.str();

            LOG_INFO_F("Using %s\n", idreference.c_str());

            std::map<uint32_t,JsonObjectDemog> nodeid_2_nodedata_map;

            // add nodeIDs 1..100 (don't start at 0, because that's an invalid (reserved) nodeid)
            for(uint32_t nodeid = 1; nodeid <= torus_size * torus_size; nodeid++)
            {
                nodeIDs.push_back( nodeid );
                JsonObjectDemog nodedata = CreateDefaultNodeDemograhics( nodeid );

                nodeid_2_nodedata_map[ nodeid ] = nodedata ;
            }
            nodedata_maps.push_back( nodeid_2_nodedata_map );

            JsonObjectDemog default_node_demographics ;
            default_node_demographics.Parse( default_node_demographics_str.c_str() );
            default_node_demographics["NodeAttributes"].Add( "InitialPopulation", (int)default_population );

            layer_defaults.push_back( default_node_demographics );

            // Generate string table to map demographics attributes to themselves so the string lookup
            // just works the same was as the case with regular demographics files
            addToStringTable(default_node_demographics, full_string_table);

            std::string demo_filename = FileSystem::Concat( EnvPtr->OutputPath, std::string("DefaultDemographics.json") );
            demographics_filenames.push_back( demo_filename );

            WriteDefaultDemographicsFile( demo_filename );
        }
    }
    catch( DetailedException& )
    {
        throw ;
    }
    catch (std::exception &e)
    {
        throw InitializationException( __FILE__, __LINE__, __FUNCTION__, e.what() );
    }
}

std::vector<std::string> NodeDemographicsFactory::GetDemographicFileNames(const ::Configuration* config)
{
    if( demographics_filenames_list.empty() )
    {
        Configure(config);
    }
    else
    {
        LOG_DEBUG("demographics_filenames_list already set, e.g. we have called SetDemographicsFileList from a component test before calling CreateNodeDemographicsFactory\n");
    }

    if( demographics_filenames_list.empty() )
    {
        throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Enable_Demographics_Initial", "1", "Demographics_Filenames", "<empty>" );
    }
    vector<string> demog_filenames;
    for (auto& filename : demographics_filenames_list)
    {
        if( filename.empty() ) continue;
        std::string demog_path = Environment::FindFileOnPath(filename);
        demog_filenames.push_back( demog_path );
    }
    return demog_filenames ;
}

bool NodeDemographicsFactory::ReadNodeData( bool isUnCompiled,
                                            int layer, 
                                            const JsonObjectDemog& rJson,
                                            const std::string& rDemographicsFilename,
                                            std::string* pLastStringValue, 
                                            std::set<std::string>* pStringTableValues )
{
    JsonObjectDemog node_list_data ;
    bool layer_has_nodes = rJson.Contains( "Nodes" );
    if( layer_has_nodes )
    {
        node_list_data = rJson["Nodes"];
        layer_has_nodes = layer_has_nodes && (node_list_data.size() > 0);
    }

    // Extract the data for each node and put it into a map so that node creation
    // can access only the nodes it needs to.
    std::map<uint32_t,JsonObjectDemog> nodeid_2_nodedata_map;
    if( layer_has_nodes )
    {
        for( int i = 0 ; i < node_list_data.size() ; i++ )
        {
            JsonObjectDemog nodedata = node_list_data[i] ;

            string nodeid_name = "NodeID" ;
            if( isUnCompiled )
            {
                // make sure the string tables are updated with the properties from the node data.
                UpdateStringTablesFromData( layer, nodedata,  pLastStringValue, pStringTableValues );
            }
            else if( full_string_table->count(nodeid_name) > 0 )
            {
                nodeid_name = (*full_string_table)[nodeid_name] ;
                if( (layer > 0) && (layer_string_value2key_tables[layer].count(nodeid_name) > 0) )
                {
                    nodeid_name = layer_string_value2key_tables[layer][nodeid_name];
                }
            }

            if( !nodedata.Contains( nodeid_name.c_str() ) )
            {
                std::stringstream msg ;
                msg << "It is missing the 'NodeID' attribute for node number " << (i+1) << ".";
                throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, rDemographicsFilename.c_str(), msg.str().c_str() );
            }
            uint32_t node_id = nodedata[ nodeid_name.c_str() ].AsInt();
            if( layer == 0 )
            {
                nodeIDs.push_back( node_id );
            }
            else if( (layer > 0) && (nodedata_maps[0].count(node_id) <= 0) )
            {
                std::stringstream msg ;
                msg << "Node number " << (i+1) << " with 'NodeID' = " << node_id << " does not exist in the base layer.  The nodes in the overlay layers must exist in the base layer." ;
                throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, rDemographicsFilename.c_str(), msg.str().c_str() );
            }
            nodeid_2_nodedata_map[ node_id ] = nodedata ;
        }
    }

    // each layer should have a map of nodeid to nodedata
    nodedata_maps.push_back( nodeid_2_nodedata_map );

    return layer_has_nodes ;
}

void NodeDemographicsFactory::UpdateStringTablesFromData( int layer, 
                                                          const JsonObjectDemog &rData,
                                                          string* pLastStringValue, 
                                                          set<string>* pStringTableValues )
{
    JsonObjectDemog json_string_table( JsonObjectDemog::JSON_OBJECT_OBJECT ) ;

    addToStringTable( rData, &json_string_table );
    UpdateStringTables( layer, json_string_table, pLastStringValue, pStringTableValues );
}

void NodeDemographicsFactory::UpdateStringTables( int layer, 
                                                  const JsonObjectDemog& rStringtable,
                                                  string* pLastStringValue, 
                                                  set<string>* pStringTableValues )
{
    release_assert( pLastStringValue );
    release_assert( pStringTableValues );

    if(layer == 0)
    {
        for( JsonObjectDemog::Iterator it = rStringtable.Begin(); it != rStringtable.End() ; ++it )
        {
            string key   = it.GetKey();
            string value = it.GetValue().AsString();

            (*full_string_table)[key] = value;
            (*pStringTableValues).insert(value);

            *pLastStringValue = value;
        }
    }
    else
    {
        release_assert( layer > 0 );
        release_assert( layer_string_sub_tables.size() > layer );
        release_assert( layer_string_value2key_tables.size() > layer );

        std::map<string, string>& string_sub_table     = layer_string_sub_tables[ layer ];
        std::map<string, string>& string_val2key_table = layer_string_value2key_tables[ layer ];

        for( JsonObjectDemog::Iterator it = rStringtable.Begin(); it != rStringtable.End() ; ++it )
        {
            string key   = it.GetKey();
            string value = it.GetValue().AsString();

            // check if the key is already in the string table
            if(full_string_table->count(key) != 0)
            {
                // if the values don't match, we need to make note so we can change the node when we read it in
                if((*full_string_table)[key].compare(value) != 0)
                {
                    string_sub_table[value] = (*full_string_table)[key];
                    string_val2key_table[ (*full_string_table)[key] ] = value ;
                }
            }
            else
            {
                // the key isn't in the table, but the value still might be...
                if( pStringTableValues->count(value) != 0)
                {
                    (*full_string_table)[key] = *pLastStringValue = GetNextStringValue( *pLastStringValue, *pStringTableValues );
                    string_sub_table[value] = *pLastStringValue;
                    string_val2key_table[*pLastStringValue] = value ;
                    pStringTableValues->insert( *pLastStringValue );
                }
                else
                {
                    (*full_string_table)[key] = value;
                    pStringTableValues->insert(value);
                }
            }
        }
    }
}

void NodeDemographicsFactory::SetIdReference( int layer, const std::string& rDemoFileName, const JsonObjectDemog& rMetadata )
{
    if( !rMetadata.Contains("IdReference") )
    {
        std::stringstream msg ;
        msg << "The 'IdReference' attribute is missing in the 'Metadata' group."  ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, rDemoFileName.c_str(), msg.str().c_str() );
    }
    if(layer == 0)
    {
        idreference = rMetadata["IdReference"].AsString();
    }
    else
    {
        string tmp_idreference = rMetadata["IdReference"].AsString();
        if(tmp_idreference.compare(idreference) != 0)
        {
            std::stringstream msg ;
            msg << "IdReference (=" << tmp_idreference << ") doesn't match base layer (=" << idreference << ").";
            throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, rDemoFileName.c_str(), msg.str().c_str() );
        }
    }
}

string NodeDemographicsFactory::GetNextStringValue(string last_value, set<string> used_values)
{
    int len = int(last_value.length());
    for(int i = len - 1; i >= 0; i--)
    {
        if(last_value[i] != 'z')
        {
            last_value = last_value.substr(0, i) + string(1, last_value[i]+1) + last_value.substr(i+1);

            if(used_values.count(last_value) == 0)
                return last_value;
            else
            {
                i++;
                continue;
            }
        }

        last_value = last_value.substr(0,i);
        for(int t = 0; t < len - i; t++)
            last_value += 'a';
    }
    
    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__ );
}

void NodeDemographicsFactory::WriteDefaultDemographicsFile( const std::string& rFilename )
{
#pragma warning( push )
#pragma warning( disable: 4996 )
    std::time_t secs = std::time(nullptr); //seconds since the Epoch
    std::string date_str = std::asctime( std::localtime( &secs ) );
    date_str = date_str.substr( 0, date_str.length()-1 ); // has '\n' end for some reason 
#pragma warning( pop )

    JsonWriterDemog writer( true ) ;
    writer << '{' 
           <<     "Metadata"
           <<     '{'
           <<          "DateCreated"  << date_str.c_str()
           <<          "Tool" << "Eradication.exe"
           <<          "Author" << "DTK"
           <<          "IdReference" << idreference.c_str()
           <<          "NodeCount" << nodeIDs.size()
           <<     '}'
           <<     "Defaults" << layer_defaults[0]
           <<     "Nodes"
           <<     '[' ;

    for( uint32_t i = 0 ; i < nodeIDs.size() ; i++ )
    {
        uint32_t node_id = nodeIDs[i] ;
        JsonObjectDemog node_json = nodedata_maps[0][node_id] ;
        writer << node_json ;
    }

    writer <<     ']'
           << '}' ;

    std::string text = writer.PrettyText();

    std::ofstream json_file;
    json_file.open( rFilename );
    if( json_file.fail() )
    {
        throw FileIOException( __FILE__, __LINE__, __FUNCTION__, rFilename.c_str() );
    }
    json_file << text ;
    json_file.close();
}

DemographicsContext* NodeDemographicsFactory::CreateDemographicsContext()
{
    DemographicsContext * dc = new DemographicsContext();
    dc->stringTable = full_string_table;

    return dc;
}


NodeDemographics* NodeDemographicsFactory::CreateNodeDemographics( JsonObjectDemog value,
                                                                   std::map<std::string, std::string> *pStringTable, 
                                                                   INodeContext *pParentNode, 
                                                                   uint32_t nodeid,
                                                                   const std::string& rValueKey,
                                                                   const std::string& rParentKey )
{
    NodeDemographics* p_nd = _new_ NodeDemographics( value, pStringTable, pParentNode, nodeid, rValueKey, rParentKey );
    return p_nd ;
}

JsonObjectDemog NodeDemographicsFactory::CreateDefaultNodeDemograhics( uint32_t nodeid )
{
    float lat = (nodeid % torus_size) * 0.008333 /* 30 arcsecs */;
    float lon = (nodeid / torus_size) * 0.008333 /* 30 arcsecs */;

    std::stringstream ss;
    ss << "{ \"NodeID\": " << nodeid << ", \"NodeAttributes\": { \"Latitude\": " << lat << ", \"Longitude\": " << lon << " } }";

    JsonObjectDemog nodedata ;
    nodedata.Parse( ss.str().c_str() );

    return nodedata ;
}


NodeDemographics* NodeDemographicsFactory::CreateNodeDemographics(INodeContext *parent_node)
{
    NodeDemographics * new_demographics = nullptr;

    JsonObjectDemog finalnodedata( JsonObjectDemog::JSON_OBJECT_OBJECT ) ;

    suids::suid node_suid = parent_node->GetSuid();

    if(nodeid_suid_map->right.count(node_suid) == 0)
    {
        //std::cerr << "Couldn't find matching NodeID for suid " << node_suid.data << endl;
        std::ostringstream msg;
        msg << "Couldn't find matching NodeID for suid " << node_suid.data << endl;
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
    }

    uint32_t nodeid = nodeid_suid_map->right.at(node_suid);

    bool found_node = false ;
    for(int i = int(demographics_filenames.size() - 1); i >= 0; i--)
    {
        JsonObjectDemog nodedata ;
        if( nodedata_maps[i].count(nodeid) > 0 )
        {
            nodedata = nodedata_maps[i][nodeid] ;
        }

        if( !nodedata.IsNull() )
        {
            TranslateNodeData(nodedata, i, finalnodedata);
            found_node = true ;
        }

        // -----------------------------------------------------------------------------------
        // --- We want to support the situation where the user defines Defaults in the overlay.
        // --- If the user does NOT define any nodes in the overlay, then the Defaults applys to all nodes.
        // --- If the user DOES define nodes in the overlay, then the Defaults in the overlay only apply to those nodes.
        // -----------------------------------------------------------------------------------
        bool apply_defaults =  !layer_defaults[i].IsNull()
                            && ((i == 0) || found_node || (nodedata_maps[i].size() == 0));

        if( apply_defaults )
        {
            LOG_DEBUG_F( "layer %d has defaults layer, translating now...\n", i );
            TranslateNodeData(layer_defaults[i], i, finalnodedata);
        }
    }
    if( !found_node )
    {
        std::ostringstream msg;
        msg << "Error: Attempted to create demographics for unknown node: " << nodeid << endl;
        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
    }

    LOG_DEBUG("Creating new NodeDemographics object with string_table\n");
    for (auto& entry : (*full_string_table))
    {
        LOG_DEBUG_F("  %s -> %s\n", entry.first.c_str(), entry.second.c_str());
    }

    new_demographics = _new_ NodeDemographics( finalnodedata, full_string_table, parent_node, nodeid, "Node", "" );

    // Have to make this check afterwards so that we can use NodeDemographics::operator[]'s
    // ability to use the string table.
    uint32_t data_nodeid = (*new_demographics)["NodeID"].AsInt();
    if( data_nodeid != nodeid )
    {
        std::stringstream msg ;
        msg << "NodeID for lookup (" << nodeid << ") does not equal the NodeID (" << data_nodeid << ") found in the data.  Is NodeOffset messed up?" ;
        throw NodeDemographicsFormatErrorException( __FILE__, __LINE__, __FUNCTION__, "UNKNOWN",  msg.str().c_str() );
    }

    return new_demographics;
}

void NodeDemographicsFactory::TranslateNodeData( const JsonObjectDemog& val, int layer, JsonObjectDemog &existing_val)
{
    // -------------------------------------------------------------------------------------------
    // --- Note: We need this method to translate every element due to the string table.
    // --- For example, if you have two compiled files, then they won't have the same two-letter
    // --- definition for each value.  We need this method to combine the data into one object
    // --- with the names based on the translation in the main string table.
    // -------------------------------------------------------------------------------------------

    if( val.IsObject() )
    {
        for( JsonObjectDemog::Iterator it = val.Begin(); it != val.End() ; ++it )
        {
            string name = it.GetKey();

            if(layer > 0 && layer_string_sub_tables[layer].count(name) != 0)
                name = layer_string_sub_tables[layer][name];

            bool attribute_exists = existing_val.Contains( name.c_str() );

            // -------------------------------------------------------------------
            // --- if the existing object is an array, do NOT merge the new values
            // --- with the existing values.
            // -------------------------------------------------------------------
            if( attribute_exists && existing_val[ name.c_str() ].IsObject() )
            {
                JsonObjectDemog v = existing_val[ name.c_str() ];
                TranslateNodeData( it.GetValue(), layer, v );
            }
            else if( !attribute_exists && it.GetValue().IsObject() )
            {
                JsonObjectDemog obj( JsonObjectDemog::JSON_OBJECT_OBJECT );
                TranslateNodeData( it.GetValue(), layer, obj );
                existing_val.Add( name, obj );
            }
            else if( !attribute_exists && it.GetValue().IsArray() )
            {
                JsonObjectDemog arr( JsonObjectDemog::JSON_OBJECT_ARRAY );
                TranslateNodeData( it.GetValue(), layer, arr );
                existing_val.Add( name, arr );
            }
            else if( !attribute_exists )
            {
                existing_val.Add( name, it.GetValue() );
            }
            else // attribute exists, but is a value or array, so attribute already there overrides this layer
            {
            }
        }
    }
    else if( val.IsArray() )
    {
        for( int i = 0 ; i < val.size() ; i++ )
        {
            if( val[i].IsObject() )
            {
                JsonObjectDemog obj( JsonObjectDemog::JSON_OBJECT_OBJECT );
                TranslateNodeData( val[i], layer, obj );
                existing_val.PushBack( obj );
            }
            else if( val[i].IsArray() )
            {
                JsonObjectDemog arr( JsonObjectDemog::JSON_OBJECT_ARRAY );
                TranslateNodeData( val[i], layer, arr );
                existing_val.PushBack( arr );
            }
            else
            {
                existing_val.PushBack( val[i] );
            }
        }
    }
    else
    {
        existing_val = val;
    }
}

void NodeDemographicsFactory::addToStringTable( const JsonObjectDemog &rNodedata, JsonObjectDemog* pStringTable )
{
    release_assert( rNodedata.IsObject() );
    release_assert( pStringTable );

    for( JsonObjectDemog::Iterator it = rNodedata.Begin(); it != rNodedata.End() ; ++it )
    {
        string name = it.GetKey();
        JsonObjectDemog value = it.GetValue();

        if( !pStringTable->Contains( name.c_str() ) )
        {
            pStringTable->Add( name, name );
        }

        if( value.IsObject() )
        {
            addToStringTable( value, pStringTable );
        }

        if( value.IsArray() )
        {
            for( int i = 0 ; i < value.size() ; i++ )
            {
                if(  value[i].IsObject() )
                {
                    addToStringTable( value[i], pStringTable );
                }
                //DMB 8-1-14: Currently, we don't have data that needs an array of arrays of objects.
            }
        }
    }
}

void NodeDemographicsFactory::addToStringTable( const JsonObjectDemog& val, std::map<std::string, std::string> * string_table)
{
    release_assert( val.IsObject() );

    for( JsonObjectDemog::Iterator it = val.Begin(); it != val.End() ; ++it )
    {
        string name = it.GetKey();
        JsonObjectDemog value = it.GetValue();

        if(string_table->count(name) == 0)
        {
            (*string_table)[name] = name;
        }

        if( value.IsObject() )
        {
            addToStringTable( value, string_table);
        }
    }
}

NodeDemographicsFactory::~NodeDemographicsFactory()
{
    delete full_string_table ;
}


NodeDemographicsDistribution* NodeDemographicsDistribution::CreateDistribution( const NodeDemographics &nd,
                                                                                int numAxes, 
                                                                                std::vector<int> &rNumPopGroups,
                                                                                std::vector< std::vector<double> > &rPopGroups, 
                                                                                std::vector<double> &rResultValues, 
                                                                                std::vector< std::vector<double> > &rDistValues)
{
    return ( _new_ NodeDemographicsDistribution(nd, numAxes, rNumPopGroups, rPopGroups, rResultValues, rDistValues ) );
}


NodeDemographicsDistribution* NodeDemographicsDistribution::CreateDistribution( const NodeDemographics& demographics )
{
    return CreateDistribution( demographics, std::vector<std::string>() );
}

NodeDemographicsDistribution* NodeDemographicsDistribution::CreateDistribution( const NodeDemographics& demographics,
                                                                                const std::string& rAxis1 )
{
    std::vector<std::string> axis_names ;
    axis_names.push_back( rAxis1 );
    return CreateDistribution( demographics, axis_names );
}

NodeDemographicsDistribution* NodeDemographicsDistribution::CreateDistribution( const NodeDemographics& demographics,
                                                                                const std::string& rAxis1,
                                                                                const std::string& rAxis2 )
{
    std::vector<std::string> axis_names ;
    axis_names.push_back( rAxis1 );
    axis_names.push_back( rAxis2 );
    return CreateDistribution( demographics, axis_names );
}

NodeDemographicsDistribution* NodeDemographicsDistribution::CreateDistribution( const NodeDemographics& demographics, 
                                                                                const std::string& rAxis1,
                                                                                const std::string& rAxis2,
                                                                                const std::string& rAxis3 )
{
    std::vector<std::string> axis_names ;
    axis_names.push_back( rAxis1 );
    axis_names.push_back( rAxis2 );
    axis_names.push_back( rAxis3 );
    return CreateDistribution( demographics, axis_names );
}

NodeDemographicsDistribution* NodeDemographicsDistribution::CreateDistribution( const NodeDemographics& demographics, 
                                                                                const std::vector<std::string>& rAxisNames )
{
    // any errors parsing the distribution info in the demographics JSON result in an exception
    try
    {
        int total_pop_groups = 1;
        std::vector<int> num_pop_groups;
        std::vector< std::vector<double> > pop_groups;

        int num_axes = 0 ;

        if( rAxisNames.size() > 0 )
        {
            bool valid_axis_names = false ;

            // -------------------------------------------------------------------------------
            // --- Verify that the user has the correct number and names of axes.
            // --- We check the names to make sure the user has the axes in the correct order.
            // --- NOTE: The user can have fewer axes than the distribution allows, but they
            // ---       still need to be in the same order.
            // -------------------------------------------------------------------------------
            JsonObjectDemog axis_names = demographics["AxisNames"].get_array();
            num_axes = axis_names.size();
            if( num_axes <= rAxisNames.size() )
            {
                valid_axis_names = true ;
                for(int axis = 0; valid_axis_names && (axis < axis_names.size()); axis++)
                {
                    string exp_axis_name = rAxisNames[ axis ] ;
                    string data_axis_name = axis_names[axis].AsString();
                    valid_axis_names = (exp_axis_name == data_axis_name);
                }
            }
            if( !valid_axis_names )
            {
                std::stringstream msg ;
                msg << demographics.GetJsonKey() << " for NodeID=" << demographics.GetNodeID() << " has 'AxisNames' with: " ;
                msg << CreateListOfNames( axis_names ) << ".  " ;
                msg << "Axis names must be the following and in the given order: " ;
                msg << CreateListOfNames( rAxisNames ) << "." ;
                throw NodeDemographicsFormatErrorException(  __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.str().c_str() );
            }

            // read axis scale-factors
            JsonObjectDemog axis_scale_factors = demographics["AxisScaleFactors"].get_array();
            if(axis_scale_factors.size() != num_axes)
            {
                std::stringstream msg ;
                msg << demographics.GetJsonKey() << " for NodeID=" << demographics.GetNodeID() << " has 'AxisScaleFactors' with " << axis_scale_factors.size() << " factor(s).  It must have one factor for each axis." ;
                throw NodeDemographicsFormatErrorException(  __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.str().c_str() );
            }

            JsonObjectDemog pop_groups_orig = demographics["PopulationGroups"].get_array();
            if( pop_groups_orig.size() != num_axes )
            {
                std::stringstream msg ;
                msg << demographics.GetJsonKey() << " for NodeID=" << demographics.GetNodeID() << " has 'PopulationGroups' with " << pop_groups_orig.size() << " array(s).  It must have one array for each axis." ;
                throw NodeDemographicsFormatErrorException(  __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.str().c_str() );
            }

            for(int i = 0; i < pop_groups_orig.size(); i++)
            {
                // ---------------------------------------------------------------------------------------------
                // --- Extract the number of arrays and elements so that we can use the information later
                // --- to ensure that ResultValues and DistributionValues have the correct number of dimensions.
                // ---------------------------------------------------------------------------------------------
                int num_elements = pop_groups_orig[i].size();
                num_pop_groups.push_back(num_elements);
                total_pop_groups *= num_elements;

                pop_groups.push_back(std::vector<double>());
                for(int j = 0; j < pop_groups_orig[i].size(); j++)
                    pop_groups[i].push_back(float(pop_groups_orig[i][j].AsDouble()));
            }

            for(int axis = 0; axis < num_axes; axis++)
            {
                int num_groups = pop_groups[axis].size();

                // apply scale-factor for the current axis
                double scale_factor = axis_scale_factors[axis].AsDouble();
                for(int group = 0; group < num_groups; group++)
                    pop_groups[axis][group] *= scale_factor;
            }
        }

        std::vector<double> result_values;
        JsonObjectDemog result_values_orig = demographics["ResultValues"].get_array();

        double result_scale_factor = demographics["ResultScaleFactor"].AsDouble();

        std::vector< std::vector<double> > dist_values;
        if( demographics.string_table->count("DistributionValues") != 0  &&  demographics.Contains("DistributionValues") )
        {
            // -------------------------------------------------------------------------------------------
            // --- Create the vector indicating how many elements should be expected in the distribution
            // --- That is, create the list of the number of arrays and array elements in the distribution.
            // -------------------------------------------------------------------------------------------
            std::vector<int> num_elements_dist ;
            for( int j = 0 ; j < num_pop_groups.size() ; j++ )
            {
                num_elements_dist.push_back( num_pop_groups[j] );
            }
            num_elements_dist.push_back( result_values_orig.size() );

            dist_values.resize(total_pop_groups);
            JsonObjectDemog dist_values_orig = demographics["DistributionValues"].get_array();
            CheckArraySize( true, demographics.GetJsonKey(), demographics.GetNodeID(),  "DistributionValues", num_elements_dist, 0, -1, -1, dist_values_orig );

            flattenDist(dist_values_orig, dist_values, 1, 0);

            result_values.resize(dist_values[0].size());
            applyResultScaleFactor(result_values_orig, result_values, result_scale_factor, 1, 0);
        }
        else
        {
            CheckArraySize( false, demographics.GetJsonKey(), demographics.GetNodeID(),  "ResultValues", num_pop_groups, 0, -1, -1, result_values_orig );

            result_values.resize(total_pop_groups);
            applyResultScaleFactor(result_values_orig, result_values, result_scale_factor, 1, 0);
        }

        // create the NodeDemographicsDistribution pointer to return
        return (_new_ NodeDemographicsDistribution(demographics, num_axes, num_pop_groups, pop_groups, result_values, dist_values));
    }
    catch(DetailedException&)
    {
        throw ;
    }
    catch(std::exception &e)
    {
        std::stringstream s ;
        s << "An exception occured while parsing '" << demographics.GetJsonKey() << "'. Error: " << e.what() ;
        throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, s.str().c_str() );
    }
}

std::string NodeDemographicsDistribution::CreateListOfNames( const std::vector<std::string>& rAxisNames )
{
    if( rAxisNames.size() <= 0 )
    {
        return std::string("");
    }

    std::string msg = "'" ;
    for( auto name : rAxisNames )
    {
        msg += name + "', '" ;
    }
    msg = msg.substr( 0, msg.size()-3 );
    return msg ;
}

std::string NodeDemographicsDistribution::CreateListOfNames( const JsonObjectDemog& rAxisNames )
{
    assert( rAxisNames.IsArray() );
    if( rAxisNames.size() <= 0 )
    {
        return std::string("");
    }

    std::string msg = "'" ;
    for( int i = 0 ; i < rAxisNames.size() ; i++ )
    {
        msg += std::string(rAxisNames[i].AsString()) + "', '" ;
    }
    msg = msg.substr( 0, msg.size()-3 );
    return msg ;
}

void NodeDemographicsDistribution::CheckArraySize( bool isCheckingDistributionValues,
                                                   const std::string& rDistName, 
                                                   uint32_t nodeID, 
                                                   const std::string& rArrayName,
                                                   const std::vector<int>& rNumPopGroups, 
                                                   int depth,
                                                   int iThElement,
                                                   int jThElement,
                                                   const JsonObjectDemog& rJsonArray )
{
    release_assert( rNumPopGroups.size() > 0 );

    if( rJsonArray.size() != rNumPopGroups[depth] )
    {
        std::stringstream msg ;
        msg << rDistName << " for NodeID=" << nodeID << " has invalid '" << rArrayName << "' array.  " ;
        if( depth == 0 )
        {
            msg << rArrayName  ;
        }
        else if( depth == 1 )
        {
            msg << rArrayName << "[" << jThElement << "]" ;
        }
        else if( depth == 2 )
        {
            msg << rArrayName << "[" << iThElement << "]" << "[" << jThElement << "]" ;
        }
        msg << " has " <<  rJsonArray.size() << " elements when it should have " <<  rNumPopGroups[depth] << "." ;
        if( isCheckingDistributionValues )
        {
            msg << "  The outer number of arrays must match that in 'PopulationGroups' with the inner most arrays having the same number of elements as the 'ResultValues'." ;
        }
        else
        {
            msg << "  The number of elements must match that in 'PopulationGroups'." ;
        }
        throw NodeDemographicsFormatErrorException(  __FILE__, __LINE__, __FUNCTION__, "UNKNOWN", msg.str().c_str() );
    }

    if( rNumPopGroups.size() > depth+1 )
    {
        iThElement = jThElement ;
        for( int j = 0 ; j < rNumPopGroups[depth] ; j++ )
        {
            CheckArraySize( isCheckingDistributionValues, rDistName, nodeID, rArrayName, rNumPopGroups, depth+1, iThElement, j, rJsonArray[j] );
        }
    }
}

float NodeDemographicsDistribution::DrawFromDistribution(float randdraw, ...) const
{
    static vector<float> groupvals;
    groupvals.resize(num_axes);

    va_list groupvals_valist;
    va_start(groupvals_valist, randdraw);

    for(int axis = 0; axis < num_axes; axis++)
        groupvals[axis] = float(va_arg(groupvals_valist, double)); // can't pass floats in varargs for some reason...

    va_end(groupvals_valist);

    float retDrawVal = pickDistributionValue(groupvals, randdraw );
    LOG_DEBUG_F("Distribution draw value = %f\n", retDrawVal);
    return retDrawVal;
}

float NodeDemographicsDistribution::DrawResultValue(double axis0value, ...) const
{
    static vector<float> groupvals;
    groupvals.resize(num_axes);

    groupvals[0] = float(axis0value);

    va_list groupvals_valist;
    va_start(groupvals_valist, axis0value);

    for(int axis = 1; axis < num_axes; axis++)
        groupvals[axis] = float(va_arg(groupvals_valist, double)); // can't pass floats in varargs for some reason...

    va_end(groupvals_valist);

    float retDrawVal = pickDistributionValue(groupvals);
    LOG_DEBUG_F("Result draw value = %f\n", retDrawVal);
    return retDrawVal;
}

float NodeDemographicsDistribution::pickDistributionValue(vector<float> &groupvals, float randdraw, int start_axis, int dim_span, int slot) const
{
    if(start_axis < num_axes)
    {
        // determine population-group(s) to use for this axis
        int num_groups = num_pop_groups[start_axis];
        int i;
        for(i = 0; i < num_groups; i++)
        {
            if(groupvals[start_axis] <= pop_groups[start_axis][i])
                break;
        }

        // could use lower_bound() instead, but it doesn't seem any faster for size of distributions we're using...
        //i = lower_bound( pop_groups[start_axis].begin(), pop_groups[start_axis].end(), groupvals[start_axis] ) - pop_groups[start_axis].begin();

        if(i == 0)
        {
            float ret_val = pickDistributionValue(groupvals, randdraw, start_axis + 1, dim_span * num_groups, slot + (dim_span * 0));
            return ret_val ;
        }
        else if(i == num_groups)
        {
            float ret_val = pickDistributionValue(groupvals, randdraw, start_axis + 1, dim_span * num_groups, slot + (dim_span * (i - 1)));
            return ret_val ;
        }
        else
        {
            float val1 = pickDistributionValue(groupvals, randdraw, start_axis + 1, dim_span * num_groups, slot + (dim_span * (i - 1)));
            float val2 = pickDistributionValue(groupvals, randdraw, start_axis + 1, dim_span * num_groups, slot + (dim_span * i));

            // calculate linear-interpolation for val1, val2
            float ret_val = float(val1 + ((groupvals[start_axis] - pop_groups[start_axis][i -1]) * (val2 - val1) / (pop_groups[start_axis][i] - pop_groups[start_axis][i - 1])));
            return ret_val ;
        }
    }
    else
    {
        if(dist_values.size() != 0)
        {
            const std::vector<double> &cdf = dist_values[slot];

            int i;
            for(i = 0; i < cdf.size(); i++)
                if(randdraw <= cdf[i])  break;

            if(i == 0)
                return (float)(result_values[0]);
            else if(i == cdf.size())
                return (float)(result_values[i - 1]);
            else
                return (float)(result_values[i - 1] + ((randdraw - cdf[i - 1]) * (result_values[i] - result_values[i - 1]) / (cdf[i] - cdf[i - 1])));
        }
        else
        {
            return float(result_values[slot]);
        }
    }
}

void NodeDemographicsDistribution::applyResultScaleFactor( const JsonObjectDemog &_result_values_orig, std::vector<double> &_result_values, double scale_factor, int dim_span, int slot)
{
    for(int group = 0; group < _result_values_orig.size(); group++)
    {
        if(_result_values_orig[group].IsArray() )
            applyResultScaleFactor(_result_values_orig[group], _result_values, scale_factor, dim_span * _result_values_orig.size(), slot + (dim_span * group));
        else
            _result_values[slot + (group * dim_span)] = _result_values_orig[group].AsDouble() * scale_factor;
    }
}

void NodeDemographicsDistribution::flattenDist( const JsonObjectDemog &_dist_values_orig, std::vector< std::vector<double> > &_dist_values, int dim_span, int slot)
{
    for(int group = 0; group < _dist_values_orig.size(); group++)
    {
        if(_dist_values_orig[group].IsArray())
            flattenDist(_dist_values_orig[group], _dist_values, dim_span * _dist_values_orig.size(), slot + (dim_span * group));
        else
            _dist_values[slot].push_back( _dist_values_orig[group].AsDouble() );
    }
}

bool NodeDemographicsDistribution::operator==( const NodeDemographicsDistribution& rThat ) const 
{
    if( !NodeDemographics::operator==( rThat ) ) return false ;

    if( this->num_axes != rThat.num_axes ) return false ;

    if( this->num_pop_groups.size() != rThat.num_pop_groups.size() ) return false ;
    for( int i = 0 ; i < this->num_pop_groups.size() ; i++ )
    {
        if( this->num_pop_groups[i] != rThat.num_pop_groups[i] ) return false ;
    }

    if( this->pop_groups.size() != rThat.pop_groups.size() ) return false ;
    for( int i = 0 ; i < this->pop_groups.size() ; i++ )
    {
        if( this->pop_groups[i].size() != rThat.pop_groups[i].size() ) return false ;
        for( int j = 0 ; j < this->pop_groups[i].size() ; j++ )
        {
            if( fabs( this->pop_groups[i][j] - rThat.pop_groups[i][j] ) > EPSILON ) return false ;
        }
    }

    if( this->result_values.size() != rThat.result_values.size() ) return false ;
    for( int i = 0 ; i < this->result_values.size() ; i++ )
    {
        if( fabs( this->result_values[i] - rThat.result_values[i] ) > EPSILON ) return false ;
    }

    if( this->dist_values.size() != rThat.dist_values.size() ) return false ;
    for( int i = 0 ; i < this->dist_values.size() ; i++ )
    {
        if( this->dist_values[i].size() != rThat.dist_values[i].size() ) return false ;
        for( int j = 0 ; j < this->dist_values[i].size() ; j++ )
        {
            if( fabs( this->dist_values[i][j] - rThat.dist_values[i][j] ) > EPSILON ) return false ;
        }
    }

    return true ;
}

bool NodeDemographicsDistribution::operator!=( const NodeDemographicsDistribution& rThat ) const
{
    return !(*this == rThat);
}

void NodeDemographics::validateIPTransition() const
{
    std::string key_list[] = { "To", "From", "Coverage", "Type", IP_PROBABILITY_KEY, IP_WHEN_KEY };
    for( auto key : key_list )
    {
        if( !Contains( key ) )
        {
            throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, NodeDemographicsFactory::GetDemographicsFileList()[0].c_str(), key.c_str() );
        }
    }

    if( !(*this)[ IP_WHEN_KEY ].Contains( "Start" ) )
    {
        throw MissingParameterFromConfigurationException( __FILE__, __LINE__, __FUNCTION__, NodeDemographicsFactory::GetDemographicsFileList()[0].c_str(), "Start" );
    }
}

}

// a step towards limiting the lookup in Node for demographic_distributions by arbitrary keys
const std::string Kernel::NodeDemographicsDistribution::FertilityDistribution       = "FertilityDistribution";
const std::string Kernel::NodeDemographicsDistribution::ImmunityDistribution        = "ImmunityDistribution";
const std::string Kernel::NodeDemographicsDistribution::MortalityDistribution       = "MortalityDistribution";
const std::string Kernel::NodeDemographicsDistribution::MortalityDistributionMale   = "MortalityDistributionMale";
const std::string Kernel::NodeDemographicsDistribution::MortalityDistributionFemale = "MortalityDistributionFemale";
const std::string Kernel::NodeDemographicsDistribution::AgeDistribution             = "AgeDistribution";

const std::string Kernel::NodeDemographicsDistribution::HIVCoinfectionDistribution          = "HIVCoinfectionDistribution";
const std::string Kernel::NodeDemographicsDistribution::HIVMortalityDistribution          = "HIVMortalityDistribution";

const std::string Kernel::NodeDemographicsDistribution::maternal_antibody_distribution1     = "maternal_antibody_distribution1";
const std::string Kernel::NodeDemographicsDistribution::maternal_antibody_distribution2     = "maternal_antibody_distribution2";
const std::string Kernel::NodeDemographicsDistribution::maternal_antibody_distribution3     = "maternal_antibody_distribution3";
const std::string Kernel::NodeDemographicsDistribution::mucosal_memory_distribution1        = "mucosal_memory_distribution1";
const std::string Kernel::NodeDemographicsDistribution::mucosal_memory_distribution2        = "mucosal_memory_distribution2";
const std::string Kernel::NodeDemographicsDistribution::mucosal_memory_distribution3        = "mucosal_memory_distribution3";
const std::string Kernel::NodeDemographicsDistribution::humoral_memory_distribution1        = "humoral_memory_distribution1";
const std::string Kernel::NodeDemographicsDistribution::humoral_memory_distribution2        = "humoral_memory_distribution2";
const std::string Kernel::NodeDemographicsDistribution::humoral_memory_distribution3        = "humoral_memory_distribution3";
const std::string Kernel::NodeDemographicsDistribution::fake_time_since_last_infection_distribution        = "fake_time_since_last_infection_distribution";

const std::string Kernel::NodeDemographicsDistribution::MSP_mean_antibody_distribution         = "MSP_mean_antibody_distribution";
const std::string Kernel::NodeDemographicsDistribution::nonspec_mean_antibody_distribution     = "nonspec_mean_antibody_distribution";
const std::string Kernel::NodeDemographicsDistribution::PfEMP1_mean_antibody_distribution      = "PfEMP1_mean_antibody_distribution";
const std::string Kernel::NodeDemographicsDistribution::MSP_variance_antibody_distribution     = "MSP_variance_antibody_distribution";
const std::string Kernel::NodeDemographicsDistribution::nonspec_variance_antibody_distribution = "nonspec_variance_antibody_distribution";
const std::string Kernel::NodeDemographicsDistribution::PfEMP1_variance_antibody_distribution  = "PfEMP1_variance_antibody_distribution";

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, DemographicsContext& dc, const unsigned int /* file_version */)
    {
        ar & dc.stringTable;
    }

    template<class Archive>
    void serialize(Archive & ar, Kernel::NodeDemographics &nd, const unsigned int /* file_version */)
    {
        std::string s;
        if (typename Archive::is_loading())
        {
            ar & s;
            json_spirit::read(s, nd.value);
        }
        else if(typename Archive::is_saving())
        {
            s = json_spirit::write(nd.value);
            ar & s;
        }
    }

    template<class Archive>
    void serialize(Archive & ar, NodeDemographicsDistribution &ndd, const unsigned int /* file_version */)
    {
        std::string s_num_pop_groups;
        std::string s_pop_groups;
        std::string s_result_values;
        std::string s_dist_values;
        json_spirit::Value v_num_pop_groups;
        json_spirit::Value v_pop_groups;
        json_spirit::Value v_result_values;

        // There's got to be a better way :(
        if (typename Archive::is_loading())
        {
            ar & s_num_pop_groups;
            json_spirit::read(s_num_pop_groups, v_num_pop_groups);
            ndd.num_pop_groups = v_num_pop_groups.get_array();

            ar & s_pop_groups;
            json_spirit::read(s_pop_groups, v_pop_groups);
            ndd.pop_groups = v_pop_groups.get_array();

            ar & s_result_values;
            json_spirit::read(s_result_values, v_result_values);
            ndd.result_values = v_result_values.get_array();

            ar & s_dist_values;
            json_spirit::read(s_dist_values, ndd.dist_values);
        }
        else if(typename Archive::is_saving())
        {
            s_num_pop_groups = json_spirit::write(json_spirit::Value(ndd.num_pop_groups));
            ar & s_num_pop_groups;

            s_pop_groups = json_spirit::write(json_spirit::Value(ndd.pop_groups));
            ar & s_pop_groups;

            s_result_values = json_spirit::write(json_spirit::Value(ndd.result_values));
            ar & s_result_values;

            s_dist_values = json_spirit::write(ndd.dist_values);
            ar & s_dist_values;
        }

        ar & ndd.num_axes;

        // Serialize base class
        ar & boost::serialization::base_object<NodeDemographics>(ndd);
    }
}
#endif
