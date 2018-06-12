/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "Log.h"
#include "IIndividualHuman.h"
#include "Interventions.h"
#include "ReportUtilities.h"
#include "Environment.h"
#include "IdmMpi.h"
#include "Serializer.h"

SETUP_LOGGING( "ReportUtilities" )

using namespace Kernel;

namespace ReportUtilities
{
    // ----------------------
    // --- Local access only
    // ----------------------

    // ------------------------------------------------------------------------
    // --- NOTE: I want to use templates for the string and double versions but
    // --- I'm not sure how to handle the IJsonObjectAdapter method call.
    // --- I tried type_traits but I still wasn't getting a good code savings.
    // ------------------------------------------------------------------------
    void InternalDeserializeVector( IJsonObjectAdapter* p_json_array, bool isSettingValuesInVector, std::vector<std::string>& rData )
    {
        if( isSettingValuesInVector && (rData.size() != p_json_array->GetSize()) )
        {
            std::stringstream ss;
            ss << "Cannot deserialize json into 1D-array because they are not the same size.  vector.size=" << rData.size() << "  json_size=" << p_json_array->GetSize();
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( unsigned int i = 0; i < p_json_array->GetSize(); ++i )
        {
            std::string value = (*p_json_array)[ IndexType( i ) ]->AsString();
            if( isSettingValuesInVector )
            {
                rData[ i ] = value;
            }
            else
            {
                rData.push_back( value );
            }
        }
    }

    void InternalDeserializeVector( IJsonObjectAdapter* p_json_array, bool isSettingValuesInVector, std::vector<double>& rData )
    {
        if( isSettingValuesInVector && (rData.size() != p_json_array->GetSize()) )
        {
            std::stringstream ss;
            ss << "Cannot deserialize json into 1D-array because they are not the same size.  vector.size=" << rData.size() << "  json_size=" << p_json_array->GetSize();
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( unsigned int i = 0; i < p_json_array->GetSize(); ++i )
        {
            double value = (*p_json_array)[ IndexType( i ) ]->AsDouble();
            if( isSettingValuesInVector )
            {
                rData[ i ] = value;
            }
            else
            {
                rData.push_back( value );
            }
        }
    }

    void InternalDeserializeVector( IJsonObjectAdapter* p_json_array, bool isSettingValuesInVector, std::vector<std::vector<double>>& rData )
    {
        if( isSettingValuesInVector && (rData.size() != p_json_array->GetSize()) )
        {
            std::stringstream ss;
            ss << "Cannot deserialize json into 2D-array because they are not the same size.  vector.size=" << rData.size() << "  json_size=" << p_json_array->GetSize();
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( unsigned int i = 0; i < p_json_array->GetSize(); ++i )
        {
            InternalDeserializeVector( (*p_json_array)[ IndexType( i ) ], isSettingValuesInVector, rData[ i ] );
        }
    }

    void InternalDeserializeVector( IJsonObjectAdapter* p_json_array, bool isSettingValuesInVector, std::vector<std::vector<std::vector<double>>>& rData )
    {
        if( isSettingValuesInVector && (rData.size() != p_json_array->GetSize()) )
        {
            std::stringstream ss;
            ss << "Cannot deserialize json into 3D-array because they are not the same size.  vector.size=" << rData.size() << "  json_size=" << p_json_array->GetSize();
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( unsigned int i = 0; i < p_json_array->GetSize(); ++i )
        {
            InternalDeserializeVector( (*p_json_array)[ IndexType( i ) ], isSettingValuesInVector, rData[ i ] );
        }
    }

    void InternalDeserializeVector( IJsonObjectAdapter* p_json_array, bool isSettingValuesInVector, std::vector<std::vector<std::vector<std::vector<double>>>>& rData )
    {
        if( isSettingValuesInVector && (rData.size() != p_json_array->GetSize()) )
        {
            std::stringstream ss;
            ss << "Cannot deserialize json into 4D-array because they are not the same size.  vector.size=" << rData.size() << "  json_size=" << p_json_array->GetSize();
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }

        for( unsigned int i = 0; i < p_json_array->GetSize(); ++i )
        {
            InternalDeserializeVector( (*p_json_array)[ IndexType( i ) ], isSettingValuesInVector, rData[ i ] );
        }
    }

    // ---------------------
    // --- Public namespace
    // ---------------------
    int GetBinIndex( float val, std::vector<float>& rValues )
    {
        if( val > rValues.back() )
        {
            return rValues.size() - 1;
        }
        else
        {
            vector<float>::const_iterator it;
            it = std::lower_bound( rValues.begin(), rValues.end(), val );
            int index = it - rValues.begin();
            return index;
        }
    }

    int GetAgeBin( float age, std::vector<float>& rAges )
    {
        float age_years = age / DAYSPERYEAR ;
        return GetBinIndex( age_years, rAges );
    }

    void SendData( const std::string& rToSend )
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

    void GetData( int fromRank, std::vector<char>& rReceive )
    {
        int32_t size = 0;
        EnvPtr->MPI.p_idm_mpi->ReceiveIntegers( &size, 1, fromRank );

        rReceive.resize( size + 1 );

        EnvPtr->MPI.p_idm_mpi->ReceiveChars( rReceive.data(), size, fromRank );
        rReceive[size] = '\0';
    }

    void SerializeVector( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<std::string>& rData )
    {
        root.Insert( pName );
        helper.JSerialize( rData, &root );
    }

    void SerializeVector( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<float>& rData )
    {
        root.Insert( pName );
        helper.JSerialize( rData, &root );
    }

    void SerializeVector( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<double>& rData )
    {
        root.Insert( pName );
        helper.JSerialize( rData, &root );
    }

    void SerializeVector( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<std::vector<double>>& rData )
    {
        root.Insert( pName );
        helper.JSerialize( rData, &root );
    }

    void SerializeVector( IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<std::vector<std::vector<double>>>& rData )
    {
        root.Insert( pName );
        root.BeginArray();
        for( std::vector<std::vector<double>>& inner : rData)
        {
            helper.JSerialize( inner, &root );
        }
        root.EndArray();
    }

    void SerializeVector(IJsonObjectAdapter& root, JSerializer& helper, const char* pName, std::vector<std::vector<std::vector<std::vector<double>>>>& rData)
    {
        root.Insert(pName);
        root.BeginArray();
        for (std::vector<std::vector<std::vector<double>>>& inner : rData)
        {
            helper.JSerialize(inner, &root);
        }
        root.EndArray();
    }

    void DeserializeVector( IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<std::string>& rData )
    {
        IJsonObjectAdapter* p_json_array = root.GetJsonArray( pName );

        InternalDeserializeVector( p_json_array, isSettingValuesInVector, rData );

        delete p_json_array;
    }

    void DeserializeVector( IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<double>& rData )
    {
        IJsonObjectAdapter* p_json_array = root.GetJsonArray( pName );

        InternalDeserializeVector( p_json_array, isSettingValuesInVector, rData );

        delete p_json_array;
    }

    void DeserializeVector( IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<std::vector<double>>& rData )
    {
        IJsonObjectAdapter* p_json_array = root.GetJsonArray( pName );

        InternalDeserializeVector( p_json_array, isSettingValuesInVector, rData );

        delete p_json_array;
    }

    void DeserializeVector( IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<std::vector<std::vector<double>>>& rData )
    {
        IJsonObjectAdapter* p_json_array = root.GetJsonArray( pName );

        InternalDeserializeVector( p_json_array, isSettingValuesInVector, rData );

        delete p_json_array;
    }

    void DeserializeVector( IJsonObjectAdapter& root, bool isSettingValuesInVector, const char* pName, std::vector<std::vector<std::vector<std::vector<double>>>>& rData )
    {
        IJsonObjectAdapter* p_json_array = root.GetJsonArray( pName );

        InternalDeserializeVector( p_json_array, isSettingValuesInVector, rData );

        delete p_json_array;
    }

    void AddVector( std::vector<double>& rThis, const std::vector<double>& rThat )
    {
        release_assert( rThis.size() ==rThat.size() );

        for( int i = 0 ; i < rThis.size(); ++i )
        {
            rThis[i] += rThat.at(i);
        }
    }

    void AddVector( std::vector<std::vector<double>>& rThis, const std::vector<std::vector<double>>& rThat )
    {
        release_assert( rThis.size() == rThat.size() );

        for( int i = 0; i < rThis.size(); ++i )
        {
            AddVector( rThis[ i ], rThat[ i ] );
        }
    }

    void AddVector( std::vector<std::vector<std::vector<double>>>& rThis, const std::vector<std::vector<std::vector<double>>>& rThat)
    {
        release_assert(rThis.size() == rThat.size());

        for (int i = 0; i < rThis.size(); ++i)
        {
            AddVector( rThis[ i ], rThat[ i ] );
        }
    }

    void AddVector( std::vector<std::vector<std::vector<std::vector<double>>>>& rThis, const std::vector<std::vector<std::vector<std::vector<double>>>>& rThat )
    {
        release_assert( rThis.size() == rThat.size() );

        for( int i = 0; i < rThis.size(); ++i )
        {
            AddVector( rThis[ i ], rThat[ i ] );
        }
    }
}