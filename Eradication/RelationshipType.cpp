
#include "stdafx.h"

#include "ConcurrencyParameters.h"
#include "Log.h"
//#include "Debug.h"
//#include "Environment.h"
//#include "RANDOM.h"
//#include "Properties.h"

SETUP_LOGGING( "RelationshipType" )


namespace Kernel
{
    std::set<std::string> GetAllowableRelationshipTypes()
    {
        std::set<std::string> allowable;

        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            allowable.insert( RelationshipType::pairs::get_keys()[ i ] );
        }
        return allowable;
    }

    std::vector<RelationshipType::Enum> GetRelationshipTypes()
    {
        std::vector<RelationshipType::Enum> rel_list;

        for( int i = 0; i < RelationshipType::COUNT; ++i )
        {
            rel_list.push_back( RelationshipType::Enum( RelationshipType::pairs::get_values()[ i ] ) );
        }

        return rel_list;
    }

    std::vector<RelationshipType::Enum> ConvertStringsToRelationshipTypes( const std::string& rParamName,
                                                                           const std::vector<std::string>& rStrings )
    {
        std::vector<RelationshipType::Enum> rel_list;

        for( auto& rel_type_str : rStrings )
        {
            int rel_type_val = RelationshipType::pairs::lookup_value( rel_type_str.c_str() );

            // I don't think I need this but just in case.
            if( rel_type_val == -1 )
            {
                std::stringstream ss;
                ss << "Unknown relationship type = " << rel_type_str;
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            if( std::find( rel_list.begin(), rel_list.end(), RelationshipType::Enum( rel_type_val ) ) != rel_list.end() )
            {
                std::stringstream ss;
                ss << "Duplicate(='" << rel_type_str << "') found in '" << rParamName << "'.  There must be one and only one of each RelationshipType.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            rel_list.push_back( RelationshipType::Enum( rel_type_val ) );
        }

        if( rel_list.size() == 0 )
        {
            rel_list = GetRelationshipTypes();
        }

        return rel_list;
    }
}
