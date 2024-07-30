
#include "stdafx.h"

#include "InterventionName.h"
#include "Debug.h"
#include "Log.h"

SETUP_LOGGING( "InterventionName" )


namespace Kernel
{
    std::map<std::string,InterventionNameInternal*> InterventionName::m_NameToInternalMap;

    InterventionName::InterventionName()
        : m_pInternal( nullptr )
    {
    }

    InterventionName::InterventionName( const std::string& rName )
        : m_pInternal( nullptr )
    {
        m_pInternal = InterventionName::CreateInternal( rName );
    }

    InterventionName::InterventionName( const char* pName )
        : m_pInternal( nullptr )
    {
        m_pInternal = InterventionName::CreateInternal( std::string( pName ) );
    }

    InterventionName::InterventionName( const InterventionName& rMaster )
        : m_pInternal( rMaster.m_pInternal )
    {
    }

    InterventionName::InterventionName( InterventionNameInternal* pInternal )
        : m_pInternal( pInternal )
    {
    }

    InterventionName::~InterventionName()
    {
    }

    InterventionName& InterventionName::operator=( const InterventionName& rName )
    {
        if( this != &rName )
        {
            this->m_pInternal = rName.m_pInternal;
        }
        return *this;
    }

    InterventionName& InterventionName::operator=( const std::string& rNameStr )
    {
        this->m_pInternal = CreateInternal( rNameStr );
        return *this;
    }

    InterventionName& InterventionName::operator=( const char* pName )
    {
        this->m_pInternal = CreateInternal( std::string( pName ) );
        return *this;
    }

    bool InterventionName::operator==( const InterventionName& rThat ) const
    {
        return (this->m_pInternal == rThat.m_pInternal);
    }

    bool InterventionName::operator!=( const InterventionName& rThat ) const
    {
        return !operator==( rThat );
    }

    bool InterventionName::operator==( const std::string& rThat ) const
    {
        if( m_pInternal == nullptr ) return false;
        return (m_pInternal->m_Name == rThat );
    }

    bool InterventionName::operator!=( const std::string& rThat ) const
    {
        return !operator==( rThat );
    }

    bool InterventionName::IsUninitialized() const
    {
        return (m_pInternal == nullptr);
    }

    const std::string& InterventionName::ToString() const
    {
        release_assert( m_pInternal != nullptr );
        return m_pInternal->m_Name;
    }

    const char* InterventionName::c_str() const
    {
        release_assert( m_pInternal != nullptr );
        return m_pInternal->m_Name.c_str();
    }

    bool InterventionName::empty() const
    {
        if( m_pInternal == nullptr )
            return true;
        else
            return m_pInternal->m_Name.empty();
    }

    int InterventionName::size() const
    {
        if( m_pInternal == nullptr )
            return 0;
        else
            return m_pInternal->m_Name.size();
    }

    void InterventionName::serialize( Kernel::IArchive& ar, InterventionName& obj )
    {
        std::string tmp;
        if( !obj.IsUninitialized() )
            tmp = obj.ToString();

        ar & tmp;

        obj = InterventionName( tmp );
    }

    InterventionNameInternal* InterventionName::CreateInternal( const std::string& rName )
    {
        if( rName.empty() )
        {
            return nullptr;
        }
        else if( m_NameToInternalMap.count( rName ) == 0 )
        {
            InterventionNameInternal* p_internal = new InterventionNameInternal( rName );
            m_NameToInternalMap[ rName ] = p_internal;
        }
        return m_NameToInternalMap.at( rName );
    }


}