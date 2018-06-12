/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "NodeSet.h"

#include "ConfigurationImpl.h"
#include "Exceptions.h"
#include "InterventionEnums.h"
#include "Log.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "NodeSetPolygon" )

namespace Kernel
{
    using namespace std;

    IMPL_QUERY_INTERFACE2(NodeSetPolygon, INodeSet, IConfigurable)

    bool
    NodeSetPolygon::Configure(const Configuration * inputJson)
    {
        initConfigTypeMap( "Vertices", &vertices_raw, Node_Polygon_Vertices_DESC_TEXT );
        initConfig( "Polygon_Format", polygon_format, inputJson, MetadataDescriptor::Enum("polygon_format", Node_Polygon_Format_DESC_TEXT, MDD_ENUM_ARGS(PolygonFormatType) ) );
        return JsonConfigurable::Configure( inputJson );
    }

    //
    // NodeSetPolygon class methods
    //
    void NodeSetPolygon::parseEmodFormat()
    {
        LOG_INFO("Parsing SHAPE polygon coordinates.\n");

        // take the string points_list
        // parse into lat-long strings
        // separate into individual lat-long coordinates, 
        // allocate polygon point array

        std::string delimiters = " ,";

        // create a vector of strings to hold each successive lat-long coordinate as a string
        vector<string> points_parser;
        std::string points_string = vertices_raw;
        std::string::size_type lastposition = points_string.find_first_not_of(delimiters, 0);
        std::string::size_type position     = points_string.find_first_of(delimiters, lastposition);

        // move through string and extract lat-long coordinates
        while (string::npos != position || string::npos != lastposition)
        {
            points_parser.push_back(points_string.substr(lastposition, position - lastposition));

            lastposition = points_string.find_first_not_of(delimiters, position);

            position = points_string.find_first_of(delimiters, lastposition);
        }

        // allocate memory and update the number of coordinates
        num_points = points_parser.size();
        points_array    = _new_ float[num_points];

        if (num_points % 2 != 0)
        {
            //// ERROR ("Error parsing polygon parameters!\n");
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Number of polygon parameters is odd." );
        }

        int faultcatcher = 0;

        for (int i = 0; i < num_points; i++)
        {
            std::istringstream temp(points_parser[i]);
            faultcatcher += (temp >> points_array[i]).fail();
        }

        if (faultcatcher)
        {
            // ERROR ("Error parsing polygon parameters!\n");
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, "Unspecified problem with polygon parameters." );
        }
        vertices_raw.clear();
    }

    // No input, use vertices_raw member variable as input.
    void NodeSetPolygon::parseGeoJsonFormat()
    {
        // vertices should be valid geojson, e.g.: [ [ 0.0,1.1 ],[ 2.2,3.3 ] ]"
        json::Object polygonJsonArray;
        std::istringstream vertices_raw_istream( vertices_raw );
        json::Reader::Read( polygonJsonArray, vertices_raw_istream );
        // poly = (json::Array&)polygonJsonArray;
    }

    bool NodeSetPolygon::Contains(INodeEventContext *nec)
    {
        LOG_DEBUG_F( "Contains node ID = %d ?\n", nec->GetId().data );
        if( vertices_raw.length() > 0 ) // clear this after parse.
        {
            // haven't parsed raw list yet.
            // Go through list and parse out.
            if( polygon_format == PolygonFormatType::SHAPE )
            {
                parseEmodFormat();
            }
            // don't need else here because enum parser already catches such errors
        }

        if( polygon_format == PolygonFormatType::SHAPE && nec->IsInPolygon( points_array, int(num_points) ) )
        {
            LOG_INFO_F( "Node ID = %d is contained within intervention polygon\n", nec->GetId().data );
            return true;
        }

        LOG_DEBUG("Polygon apparently does not contain this node.\n");

        /*else if( polygon_format == PolygonFormatType::GEOJSON && nec->IsInPolygon( poly ) )
        {
            return true;
        }*/
        return false;
    }

    std::vector<ExternalNodeId_t> NodeSetPolygon::IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids)
    {
        std::vector<ExternalNodeId_t> dummy;
        return dummy;
    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, NodeSetPolygon& nodeset, const unsigned int v)
    {
        ar & vertices_raw;
        //ar & points_array;
        ar & num_points;
        ar & polygon_format;
    }
}
#endif
