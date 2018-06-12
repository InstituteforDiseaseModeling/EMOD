/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <algorithm> // std::find()
#include <sstream>
#include "ProgramOptions.h"
#include "IdmString.h"

using namespace std;

const std::string ProgramOptions::SPACES_40(40, ' ') ;


ProgramOptions::ProgramOptions( const string& rDefaultMessage )
: defaultMessage( rDefaultMessage )
, mapLongNameToOption()
, mapShortNameToOption()
, insertionOrder()
{
}

ProgramOptions::~ProgramOptions()
{
    for( std::map<string,Option*>::iterator it = mapLongNameToOption.begin() ; it != mapLongNameToOption.end() ; it++ )
    {
        if( it->second != nullptr )
        {
            delete (it->second);
            it->second = nullptr ;
        }
    }

    // We don't delete the options in mapShortNameToOption because every entry in this map
    // should be in mapLongNameToOption.  mapShortNameToOption should be a subset of mapLongNameToOption
}

void ProgramOptions::AddOption( const string& rLongName, const string& rDescription )
{
    AddOption( rLongName, "", rDescription );
}

void ProgramOptions::AddOption( const string& rLongName, const string& rShortName, const string& rDescription )
{
    release_assert( !rLongName.empty() );
    release_assert( !rDescription.empty() );
    release_assert( mapLongNameToOption.count(rLongName) == 0 );

    Option* p_opt = new Option( rLongName, rShortName, rDescription ) ;
    mapLongNameToOption[ rLongName ] = p_opt ;
    insertionOrder.push_back( rLongName );

    if( !rShortName.empty() )
    {
        mapShortNameToOption[ rShortName ] = p_opt ;
    }
}

void ProgramOptions::AddOptionWithValue( const string& rLongName, int defaultValue, const string& rDescription )
{
    release_assert( !rLongName.empty() );
    release_assert( !rDescription.empty() );
    release_assert( mapLongNameToOption.count(rLongName) == 0 );

    OptionInt* p_opt = new OptionInt( rLongName, defaultValue, rDescription );
    mapLongNameToOption[ rLongName ] = p_opt ;
    insertionOrder.push_back( rLongName );
}

void ProgramOptions::AddOptionWithValue( const string& rLongName, int defaultValue, int implicitVal, const string& rDescription )
{
    AddOptionWithValue( rLongName, defaultValue, rDescription );

    OptionInt* p_opt = dynamic_cast<OptionInt*>(mapLongNameToOption[ rLongName ]) ;
    release_assert( p_opt );
    p_opt->hasImplicitValue = true ;
    p_opt->implicitValue = implicitVal ;
}

void ProgramOptions::AddOptionWithValue( const string& rLongName, const string& rDefaultValue, const string& rDescription )
{
    AddOptionWithValue( rLongName, "", rDefaultValue, rDescription );
}

void ProgramOptions::AddOptionWithValue( const string& rLongName, const string& rShortName, const string& rDefaultValue, const string& rDescription )
{
    release_assert( !rLongName.empty() );
    release_assert( !rDescription.empty() );
    release_assert( mapLongNameToOption.count(rLongName) == 0 );

    OptionString* p_opt = new OptionString( rLongName, rShortName, rDefaultValue, rDescription );
    mapLongNameToOption[ rLongName ] = p_opt ;
    insertionOrder.push_back( rLongName );
    if( !rShortName.empty() )
    {
        mapShortNameToOption[ rShortName ] = p_opt ;
    }
}

void ProgramOptions::AddOptionWithValue( const string& rLongName, const vector<string>& rPossibleValues,  const string& rDescription )
{
    release_assert( !rLongName.empty() );
    release_assert( !rDescription.empty() );
    release_assert( mapLongNameToOption.count(rLongName) == 0 );

    OptionListString* p_opt = new OptionListString( rLongName, rPossibleValues, rDescription );
    mapLongNameToOption[ rLongName ] = p_opt ;
    insertionOrder.push_back( rLongName );
}

bool ProgramOptions::CommandLineHas( const string& rLongName )
{
    release_assert( !rLongName.empty() );
    release_assert( mapLongNameToOption.count(rLongName) > 0 );

    Option* p_opt = GetOption( rLongName );

    return p_opt->commandLineHas ;
}

string ProgramOptions::GetCommandLineValueString( const string& rLongName )
{
    release_assert( !rLongName.empty() );

    Option* p_opt = GetOption( rLongName );

    OptionString*     p_opt_str      = dynamic_cast<OptionString*    >(p_opt);
    OptionListString* p_opt_list_str = dynamic_cast<OptionListString*>(p_opt);
    release_assert( (p_opt_str != nullptr) || (p_opt_list_str != nullptr) );

    string ret_val ;
    if( p_opt_str != nullptr )
    {
        ret_val = p_opt_str->value ;
    }
    else if( p_opt_list_str != nullptr )
    {
        ret_val = p_opt_list_str->value ;
    }

    return ret_val ;
}

string ProgramOptions::GetCommandLineValueDefaultString( const string& rLongName )
{
    release_assert( !rLongName.empty() );

    Option* p_opt = GetOption( rLongName );

    OptionString*     p_opt_str      = dynamic_cast<OptionString*    >(p_opt);
    OptionListString* p_opt_list_str = dynamic_cast<OptionListString*>(p_opt);
    release_assert( (p_opt_str != nullptr) || (p_opt_list_str != nullptr) );

    string ret_val ;
    if( p_opt_str != nullptr )
    {
        ret_val = p_opt_str->defaultValue ;
    }
    else if( p_opt_list_str != nullptr )
    {
        ret_val = p_opt_list_str->possibleValues[0] ;
    }

    return ret_val ;
}

int ProgramOptions::GetCommandLineValueInt( const string& rLongName )
{
    release_assert( !rLongName.empty() );

    Option* p_opt = GetOption( rLongName );

    OptionInt* p_opt_int = dynamic_cast<OptionInt*>(p_opt);
    release_assert( p_opt_int != nullptr );

    return p_opt_int->value ;
}

string ProgramOptions::ParseCommandLine( int argc, char *argv[] )
{
    for( int arg_i = 1 ; arg_i < argc ; arg_i++ )
    {
        Option* p_opt = nullptr ;

        IdmString arg = string( argv[arg_i] );
        vector<IdmString> sub_args = arg.split('=');
        string opt_name = sub_args[0] ;
        if( (opt_name[0] == '-') && (opt_name[1] == '-') )
        {
            string long_name = opt_name.substr( 2, opt_name.size()-2 );
            // checking count protects from adding an entry to the map unintentionally
            if( mapLongNameToOption.count( long_name ) > 0 )
            {
                p_opt = mapLongNameToOption[ long_name ] ;
            }
        }
        else if( opt_name[0] == '-' )
        {
            string short_name = opt_name.substr( 1, opt_name.size()-1 );
            // checking count protects from adding an entry to the map unintentionally
            if( mapShortNameToOption.count( short_name ) > 0 )
            {
                p_opt = mapShortNameToOption[ short_name ] ;
            }
        }

        if( p_opt == nullptr )
        {
            string errmsg = "Error parsing command line: unrecognised option '"+opt_name+"'" ;
            return errmsg ;
        }

        OptionInt*        p_opt_int      = dynamic_cast<OptionInt       *>(p_opt);
        OptionString*     p_opt_str      = dynamic_cast<OptionString    *>(p_opt);
        OptionListString* p_opt_list_str = dynamic_cast<OptionListString*>(p_opt);

        string val_str ;
        if( sub_args.size() == 2 )
        {
            // '=' was used so get the value from that
            val_str = sub_args[1] ;
        }
        else if( sub_args.size() > 2 )
        {
            string errmsg = "Error parsing command line: not properly formed option value (more than one '=') ["+arg+"]" ;
            return errmsg ;
        }
        else if( (p_opt_int      != nullptr) ||
                 (p_opt_str      != nullptr) ||
                 (p_opt_list_str != nullptr) )
        {
            // ParseValue will increment the loop counter arg_i so it can get the value for the option
            bool success = ParseValue( argc, argv, &arg_i, &val_str );

            // there might not be another argument if the option has an implicit value
            if( !success && ((p_opt_int != nullptr) && p_opt_int->hasImplicitValue) )
            {
                arg_i-- ;
            }
            else if( !success )
            {
                string errmsg = "Error parsing command line: missing value for option '"+(p_opt->longName)+"'" ;
                return errmsg ;
            }
        }

        if( p_opt_int != nullptr )
        {
            int val_int = - 1 ;
            if( p_opt_int->hasImplicitValue && val_str.empty() )
            {
                val_int = p_opt_int->implicitValue ;
            }
            else
            {
                if( val_str.empty() )
                {
                    string errmsg = "Error parsing command line: missing value for option '"+(p_opt->longName)+"'" ;
                    return errmsg ;
                }
                try
                {
                    val_int = std::stoi( val_str );
                }
                catch( std::exception&  )
                {
                    string errmsg = "Error parsing command line: illegal value("+val_str+") for option '"+(p_opt->longName)+"'" ;
                    return errmsg ;
                }
            }
            p_opt_int->value = val_int ;
        }
        else if( p_opt_str != nullptr )
        {
            p_opt_str->value = val_str ;
        }
        else if( p_opt_list_str != nullptr )
        {
            if( std::find( p_opt_list_str->possibleValues.begin(), 
                           p_opt_list_str->possibleValues.end(),
                           val_str ) == p_opt_list_str->possibleValues.end() )
            {
                string errmsg = "Error parsing command line: illegal value("+val_str+") for option '"+(p_opt->longName)+"'" ;
                return errmsg ;

            }
            p_opt_list_str->value = val_str ;
        }
        else // Option
        {
            p_opt->commandLineHas = true ;
        }
    }

    return "" ;
}

void ProgramOptions::Print( ostream& rOstream )
{
    rOstream << defaultMessage << ":" << endl ;
    for( int i = 0 ; i < insertionOrder.size() ; i++ )
    {
        string line = "  " ;
        Option* p_opt = mapLongNameToOption[ insertionOrder[i] ] ;
        if( p_opt->shortName.empty() )
        {
            line += "--" + p_opt->longName ;
        }
        else
        {
            line += "-" + p_opt->shortName + " [ --" + p_opt->longName + " ]" ;
        }

        line += AddArgInfo( p_opt );

        if( line.size() >= 40 )
        {
            line += "\n" ;
            line += SPACES_40 ;
        }
        else
        {
            while( line.size() < 40 )
            {
                line.append(" ");
            }
        }

        vector<string> desc_40 = BreakDescriptionIntoChunks( p_opt->description );
        for( int i = 0 ; i < desc_40.size() ; i++ )
        {
            line += desc_40[i] + "\n" ;
            if( (i+1) < desc_40.size() )
            {
                line += SPACES_40 ;
            }
        }
        rOstream << line ;
    }
}

string ProgramOptions::AddArgInfo( const Option* pOpt )
{
    release_assert( pOpt );

    stringstream stm ;

    const OptionInt*        p_opt_int      = dynamic_cast<const OptionInt       *>(pOpt);
    const OptionString*     p_opt_str      = dynamic_cast<const OptionString    *>(pOpt);
    const OptionListString* p_opt_list_str = dynamic_cast<const OptionListString*>(pOpt);

    if( p_opt_int != nullptr )
    {
        if( p_opt_int->hasImplicitValue )
        {
            stm << " [=arg(=" << p_opt_int->implicitValue << ")] " ;
        }
        else
        {
            stm << " arg " ;
        }
        stm << "(=" << p_opt_int->defaultValue << ")" ;
    }
    else if( p_opt_str != nullptr )
    {
        stm << " arg" ;
        if( !p_opt_str->defaultValue.empty() )
        {
            stm << " (=" << p_opt_str->defaultValue << ")" ;
        }
    }
    else if( p_opt_list_str != nullptr )
    {
        stm << " arg (=" << p_opt_list_str->possibleValues[0] << ")" ;
    }

    string line = stm.str() ;

    return line ;
}

vector<string> ProgramOptions::BreakDescriptionIntoChunks( const string& rDesc )
{
    IdmString desc = rDesc ;
    vector<IdmString> words = desc.split(' ');
    vector<string> list_line_40 ;
    string line_40 ;
    for( int i = 0 ; i < words.size() ; i++ )
    {
        line_40 += words[i] ;
        if( (i+1) < words.size() )
        {
            line_40 += " " ;
            if( (line_40.size() + 1 + words[i+1].size()) > 40 ) // the plus one is for space
            {
                list_line_40.push_back( line_40 ) ;
                line_40 = "" ;
            }
        }
    }

    list_line_40.push_back( line_40 );

    return list_line_40 ;
}

ProgramOptions::Option* ProgramOptions::GetOption( const std::string& rLongName )
{
    Option* p_opt = mapLongNameToOption[ rLongName ] ;
    release_assert( p_opt != nullptr );

    return p_opt ;
}

bool ProgramOptions::ParseValue( int argc, char* argv[], int* pArgI, std::string* pValStr )
{
    release_assert( argv );
    release_assert( pArgI );
    release_assert( pValStr );

    // increment the loop counter to get the value for the option
    (*pArgI)++ ;

    if( *pArgI >= argc )
    {
        return false;
    }

    string val_str = std::string( argv[ *pArgI ] );
    if( val_str[0] == '-' )
    {
        return false;
    }

    *pValStr = val_str ;

    return true ;
}

void ProgramOptions::Reset()
{
    for( std::map<string,Option*>::iterator it = mapLongNameToOption.begin() ; it != mapLongNameToOption.end() ; it++ )
    {
        Option*           p_opt          = it->second ;
        OptionInt*        p_opt_int      = dynamic_cast<OptionInt       *>(p_opt);
        OptionString*     p_opt_str      = dynamic_cast<OptionString    *>(p_opt);
        OptionListString* p_opt_list_str = dynamic_cast<OptionListString*>(p_opt);

        release_assert( p_opt );

        if( p_opt_int != nullptr )
        {
            p_opt_int->value = p_opt_int->defaultValue ;
        }
        else if( p_opt_str != nullptr )
        {
            p_opt_str->value = p_opt_str->defaultValue ;
        }
        else if( p_opt_list_str != nullptr )
        {
            p_opt_list_str->value = p_opt_list_str->possibleValues[0] ;
        }
        else
        {
            p_opt->commandLineHas = false ;
        }
    }
}