/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

//////////////////////////////////////////////////////////////////////////////
//
// The wrapper class for underlying json libraries
//
#pragma once

#include <string>
#include <memory>
#include <stdint.h>
#include "IdmApi.h"

namespace Kernel
{
    typedef uint32_t IndexType;

    // The purpose of the JsonObject is to wrap an underlying Json implementation.
    // The JsonObject can be many things: Object, Array, or Value (or Null).
    // The methods you can call on a JsonObject depends on the type of item it is.
    // For example, you cannot call array methods on something that is an object.
    class IDMAPI JsonObjectDemog
    {
    public:
        friend class JsonWriterDemog;
        class Iterator ;

        enum JsonObjectType
        {
            JSON_OBJECT_NULL,
            JSON_OBJECT_OBJECT,
            JSON_OBJECT_ARRAY
        };

        JsonObjectDemog( JsonObjectType jot = JSON_OBJECT_NULL );
        ~JsonObjectDemog();

        JsonObjectDemog( const JsonObjectDemog& rThat );
        JsonObjectDemog& operator=( const JsonObjectDemog& rThat );

        bool operator==( const JsonObjectDemog& rThat ) const ;
        bool operator!=( const JsonObjectDemog& rThat ) const ;

        void WriteToFile( const char* filename );

        // ------------------------------------------------------
        // --- Methods for creating an object from a string and
        // --- for creating the string from the object
        // ------------------------------------------------------
        void Parse( const char* jsBuffer, const char* filename = nullptr );
        void ParseFile( const char* filename );
        std::string ToString() const ;

        // ------------------------------------------------------
        // --- Methods for querying information about the object
        // ------------------------------------------------------
        std::string GetKey() const ;
        bool Contains(const char* key) const;
        bool IsNull() const ;
        bool IsArray() const ;
        bool IsObject() const ;
        std::string GetTypeName() const ;

        // ----------------------------------------------
        // --- Methods for getting values from an object
        // ----------------------------------------------
        JsonObjectDemog operator[](    const char* key ) const;
        JsonObjectDemog GetJsonObject( const char* key ) const;
        JsonObjectDemog GetJsonArray(  const char* key ) const;
        const char*     GetString(     const char* key ) const;
        int32_t         GetInt(        const char* key ) const;
        uint32_t        GetUint(       const char* key ) const;
        int64_t         GetInt64(      const char* key ) const;
        uint64_t        GetUint64(     const char* key ) const;
        float           GetFloat(      const char* key ) const;
        double          GetDouble(     const char* key ) const;
        bool            GetBool(       const char* key ) const;

        // -------------------------------------------
        // --- Methods for getting values from values
        // -------------------------------------------
        const char* AsString() const;
        int32_t     AsInt()    const;
        uint32_t    AsUint()   const;
        int64_t     AsInt64()  const;
        uint64_t    AsUint64() const;
        float       AsFloat()  const;
        double      AsDouble() const;
        bool        AsBool()   const;

        operator const char*() const;
        operator int32_t()     const;
        operator float()       const;
        operator bool()        const;

        // ---------------------------------------------------------
        // --- Methods for Adding and Removing items from an Object
        // ---------------------------------------------------------
        void Add( const std::string& rKey, const JsonObjectDemog& rValue );
        void Add( const char*        pKey, const JsonObjectDemog& rValue );
        void Add( const std::string& rKey, const std::string&     rStrValue );
        void Add( const char*        pKey, const char*            pStrValue );
        void Add( const std::string& rKey, double                 val );
        void Add( const char*        pKey, double                 val );
        void Add( const std::string& rKey, int                    val );
        void Add( const char*        pKey, int                    val );

        void Remove( const Iterator& rIterator );

        // -------------------------------------
        // --- Methods for manipulating an Array
        // -------------------------------------
        size_t size() const;
        JsonObjectDemog operator[]( IndexType index ) const;
        void PushBack( const JsonObjectDemog& rValue );
        void PushBack( const std::string& rStr );
        //void PopBack() ;

        // -------------------------------------------------------------
        // --- An Iterator lets one cycle through the items of an object
        // -------------------------------------------------------------
        class IDMAPI Iterator
        {
        public:
            ~Iterator();

            bool operator==( const Iterator& rThat ) const ;
            bool operator!=( const Iterator& rThat ) const ;

            Iterator& operator++() ; //prefix

            std::string GetKey() const ;
            JsonObjectDemog GetValue() const ;

        private:
            friend class JsonObjectDemog;

            Iterator( void* pData, std::shared_ptr<void> pDocument, void* pValue );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
            void*                 m_pData ;
            std::shared_ptr<void> m_pDocument ;
            void*                 m_pValue ;
#pragma warning( pop )

        };

        Iterator Begin() const;
        Iterator End() const ;

    private:
        JsonObjectDemog( const std::string& rKey, void* pValue, std::shared_ptr<void> pDocument );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        std::string m_Key ;
        std::shared_ptr<void> m_pDocument ;
        void* m_pValue ;
#pragma warning( pop )
    };

    // A JsonWriter object allows one to create json formatted text
    class IDMAPI JsonWriterDemog
    {
    public:
        JsonWriterDemog(bool);
        ~JsonWriterDemog();

        const char* Text() const;
        char* PrettyText() const;

        JsonWriterDemog& operator <<(const char     bracket);
        JsonWriterDemog& operator <<(const char*    value);
        JsonWriterDemog& operator <<(const int32_t  value);
        JsonWriterDemog& operator <<(const uint32_t value);
        JsonWriterDemog& operator <<(const int64_t  value);
        JsonWriterDemog& operator <<(const uint64_t value);
        JsonWriterDemog& operator <<(const float    value);
        JsonWriterDemog& operator <<(const double   value);
        JsonWriterDemog& operator <<(const bool     value);
        JsonWriterDemog& operator <<(const JsonObjectDemog& rValue );

    private:
        void* m_pBuffer ;
        void* m_pWriter ;
    };
}
