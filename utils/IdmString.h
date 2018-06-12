/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once 

#include <string>
#include <cstring>
#include <vector>
#include <map>

class IdmString : public std::string
{
    //typedef std::string tInternalStringType;
    typedef IdmString tInternalStringType;
    typedef std::vector< tInternalStringType > tFragmentsType;
    typedef std::map< char, tFragmentsType > tFragmentsMapType;

    public:
        IdmString( const char * init_string )
            :std::string( init_string )
        {
            //fragments.resize(0);
        }

        IdmString( const std::string& init_string )
            :std::string( init_string )
        {
            //fragments.resize(0);
        }

        tFragmentsType& split( const char limiter = ':' )
        {
            if( fragments[ limiter ].size() == 0 )
            {
                std::vector< int > locations;
                int left = 0;
                locations.push_back( left );
                while( (*this).find( limiter, left ) != std::string::npos )
                {
                    left = (int)(*this).find( limiter, left ) + 1;
                    locations.push_back( left );
                }
                locations.push_back( (int)(*this).size()+1 );
                for( int index = 0; index < locations.size()-1; index++ )
                {
                    tInternalStringType fragment = ( (*this).substr( locations[index], locations[index+1]-locations[index]-1 ) ).c_str();
                    fragments[ limiter ].push_back( fragment );
                }
            }
            return fragments[ limiter ];
        }

        bool contains( const char *needle )
        {
            if( (*this).find( needle ) == std::string::npos )
            {
                return false;
            }
            else
            {
                return true;
            }
        }

    private:
        tFragmentsMapType fragments;
};
