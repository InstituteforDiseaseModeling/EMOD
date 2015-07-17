/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "Properties.h"

const char* IP_PROBABILITY_KEY        = "Probability_Per_Timestep";
const char* IP_REVERSION_KEY          = "Timesteps_Until_Reversion";
const char* IP_INIT_KEY               = "Initial_Distribution";
const char* IP_AGE_KEY                = "Age_In_Years_Restriction";
const char* IP_WHEN_KEY               = "Timestep_Restriction";
const char* IP_AGE_BIN_KEY            = "Age_Bin_Edges_In_Years";

const char* IP_KEY                    = "IndividualProperties";
const char* IP_NAME_KEY               = "Property";
const char* IP_VALUES_KEY             = "Values";
const char* TRANSMISSION_MATRIX_KEY   = "TransmissionMatrix";
const char* ROUTE_KEY                 = "Route";
const char* TRANSMISSION_DATA_KEY     = "Matrix";

std::string PropertiesToString( const tProperties& properties, 
                                const char propValSeparator, 
                                const char propSeparator )
{
    std::string propertyString;
    for (const auto& entry : properties)
    {
        const std::string& key   = entry.first;
        const std::string& value = entry.second;
        propertyString += key + propValSeparator + value + propSeparator ;
    }

    if( !propertyString.empty() )
    {
#ifdef WIN32
        propertyString.pop_back();
#else
        propertyString.resize(propertyString.size() - 1);
#endif
    }

    return propertyString;
}

std::string PropertiesToString( const tProperties& properties )
{
    return PropertiesToString( properties, ':', ',' );
}

std::string PropertiesToStringCsvFriendly( const tProperties& properties )
{
    return PropertiesToString( properties, '-', ';' );
}
