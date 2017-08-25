/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "../interventions/IDrug.h"
#include "Log.h"
#include "IIndividualHuman.h"
#include "Interventions.h"
#include "ReportUtilities.h"
#include "Environment.h"
#include "IdmMpi.h"
#include "Serializer.h"

SETUP_LOGGING( "ReportUtilities" )

using namespace Kernel;

int ReportUtilities::GetAgeBin( float age, std::vector<float>& rAges )
{
    float age_years = age / DAYSPERYEAR ;
    if( age_years > rAges[rAges.size()-1] )
    {
        LOG_WARN_F("Age_Bins not large enough for population, found age(years)=%f and Age_Bin.end=%f.  Putting in last bin.\n", age_years, rAges[rAges.size()-1] );
        return rAges.size()-1 ;
    }
    else
    {
        vector<float>::const_iterator it;
        it = std::lower_bound(rAges.begin(), rAges.end(), age_years );
        int agebin_idx = it - rAges.begin();
        return agebin_idx;
    }
}

void ReportUtilities::SendData( const std::string& rToSend )
{
    uint32_t size = rToSend.size();

    IdmMpi::Request size_request;
    EnvPtr->MPI.p_idm_mpi->SendIntegers( &size, 1, 0, &size_request );


    IdmMpi::Request data_request;
    EnvPtr->MPI.p_idm_mpi->SendChars( rToSend.c_str(), size, 0, &data_request );

    IdmMpi::RequestList request_list;
    request_list.Add( size_request );
    request_list.Add( data_request );

    EnvPtr->MPI.p_idm_mpi->WaitAll( request_list );
}

void ReportUtilities::GetData( int fromRank, std::vector<char>& rReceive )
{
    int32_t size = 0;
    EnvPtr->MPI.p_idm_mpi->ReceiveIntegers( &size, 1, fromRank );

    rReceive.resize( size + 1 );

    EnvPtr->MPI.p_idm_mpi->ReceiveChars( rReceive.data(), size, fromRank );
    rReceive[size] = '\0';
}

void ReportUtilities::SerializeVector( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<double>& rData )
{
    root.Insert( pName );
    helper.JSerialize( rData, &root );
}

void ReportUtilities::SerializeVector( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<float>& rData )
{
    root.Insert( pName );
    helper.JSerialize( rData, &root );
}

void ReportUtilities::SerializeVector2D( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<std::vector<double>>& rData )
{
    root.Insert( pName );
    helper.JSerialize( rData, &root );
}

void ReportUtilities::SerializeVector3D( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<std::vector<std::vector<double>>>& rData )
{
    root.Insert( pName );
    root.BeginArray();
    for( std::vector<std::vector<double>>& inner : rData)
    {
        helper.JSerialize( inner, &root );
    }
    root.EndArray();
}

void ReportUtilities::DeserializeVector( IJsonObjectAdapter& root, bool isSettingValue, const char* pName, std::vector<double>& rData )
{
    IJsonObjectAdapter* p_json_array = root.GetJsonArray( pName );

    if( isSettingValue && (rData.size() != p_json_array->GetSize()) )
    {
        std::stringstream ss;
        ss << "Cannot deserialize json into array because they are not the same size.  vector.size=" << rData.size() << "  json_size=" << p_json_array->GetSize();
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
    }

    for( unsigned int i = 0 ; i < p_json_array->GetSize(); ++i )
    {
        double value = (*p_json_array)[IndexType(i)]->AsDouble();
        if( isSettingValue )
        {
            rData[i] = value;
        }
        else
        {
            rData.push_back( value );
        }
    }
    delete p_json_array;
}

void ReportUtilities::AddVector( std::vector<double>& rThis, const std::vector<double>& rThat )
{
    release_assert( rThis.size() ==rThat.size() );

    for( int i = 0 ; i < rThis.size(); ++i )
    {
        rThis[i] += rThat.at(i);
    }
}


