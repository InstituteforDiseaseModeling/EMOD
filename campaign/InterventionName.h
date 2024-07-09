
#pragma once

#include <string>

#include "IArchive.h"

namespace Kernel
{
    class InterventionNameInternal
    {
    public:
        std::string m_Name;

        InterventionNameInternal( const std::string& rName )
            : m_Name( rName )
        {
        }
    };

    // The purpose of the InterventionName class is to reduce the amount of time
    // spent doing string compares and waiting for the memory of the interention
    // to load.
    class InterventionName
    {
    public:
        InterventionName();
        explicit InterventionName( const std::string& rName  );
        explicit InterventionName( const char* pName );
        InterventionName( const InterventionName& rMaster );
        ~InterventionName();

        InterventionName& operator=( const InterventionName& rName );
        InterventionName& operator=( const std::string& rNameStr );
        InterventionName& operator=( const char* pName );

        bool operator==( const InterventionName& rThat ) const;
        bool operator!=( const InterventionName& rThat ) const;

        bool operator==( const std::string& rThat ) const;
        bool operator!=( const std::string& rThat ) const;

        // Returns true if the InterventionName has not been intialized
        bool IsUninitialized() const;

        // Returns the string value of the event
        const std::string& ToString() const;
        const char* c_str() const;
        bool empty() const;
        int size() const;

        static void serialize( Kernel::IArchive& ar, InterventionName& obj );
    protected:
        InterventionName( InterventionNameInternal* pInternal );

        InterventionNameInternal* m_pInternal;

    private:
        static InterventionNameInternal* CreateInternal( const std::string& rName );

        static std::map<std::string,InterventionNameInternal*> m_NameToInternalMap;
    };
}