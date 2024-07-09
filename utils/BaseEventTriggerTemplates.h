
#pragma once

#include "BaseEventTrigger.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- BaseEventTrigger
    // ------------------------------------------------------------------------

    template<class Trigger, class Factory>
    BaseEventTrigger<Trigger,Factory>::BaseEventTrigger()
        : m_pInternal( nullptr )
    {
    }

    template<class Trigger, class Factory>
    BaseEventTrigger<Trigger, Factory>::BaseEventTrigger( const std::string &init_str )
        : m_pInternal( Factory::GetInstance()->GetEventTriggerInternal( init_str ) )
    {
    }

    template<class Trigger, class Factory>
    BaseEventTrigger<Trigger, Factory>::BaseEventTrigger( const char *init_str )
        : m_pInternal( Factory::GetInstance()->GetEventTriggerInternal( init_str ) )
    {
    }

    template<class Trigger, class Factory>
    BaseEventTrigger<Trigger, Factory>::BaseEventTrigger( EventTriggerInternal* peti )
        : m_pInternal( peti )
    {
    }

    template<class Trigger, class Factory>
    BaseEventTrigger<Trigger, Factory>::~BaseEventTrigger()
    {
        m_pInternal = nullptr;
    }

    template<class Trigger, class Factory>
    Trigger& BaseEventTrigger<Trigger, Factory>::operator=( const Trigger& rTrigger )
    {
        this->m_pInternal = rTrigger.m_pInternal;
        return *this;
    }

    template<class Trigger, class Factory>
    Trigger& BaseEventTrigger<Trigger, Factory>::operator=( const std::string& rTriggerStr )
    {
        m_pInternal = Factory::GetInstance()->GetEventTriggerInternal( rTriggerStr );
        return *this;
    }

    template<class Trigger, class Factory>
    bool BaseEventTrigger<Trigger, Factory>::operator==( const Trigger& rThat ) const
    {
        return (this->m_pInternal == rThat.m_pInternal);
    }

    template<class Trigger, class Factory>
    bool BaseEventTrigger<Trigger, Factory>::operator!=( const Trigger& rThat ) const
    {
        return (this->m_pInternal != rThat.m_pInternal);
    }

    template<class Trigger, class Factory>
    const std::string& BaseEventTrigger<Trigger, Factory>::ToString() const
    {
        if( m_pInternal == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Cannot use ToString().  EventTrigger is uninitialized." );
        }
        return m_pInternal->m_Name;
    }

    template<class Trigger, class Factory>
    const char* BaseEventTrigger<Trigger, Factory>::c_str() const
    {
        if( m_pInternal == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Cannot use ToString().  EventTrigger is uninitialized." );
        }
        return m_pInternal->m_Name.c_str();
    }

    template<class Trigger, class Factory>
    int BaseEventTrigger<Trigger, Factory>::GetIndex() const
    {
        if( m_pInternal == nullptr )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Cannot use ToString().  EventTrigger is uninitialized." );
        }
        return m_pInternal->m_Index;
    }

    template<class Trigger, class Factory>
    bool BaseEventTrigger<Trigger, Factory>::IsUninitialized() const
    {
        return ( m_pInternal == nullptr );
    }

    template<class Trigger, class Factory>
    void BaseEventTrigger<Trigger, Factory>::serialize( Kernel::IArchive& ar, Trigger& obj )
    {
        std::string tmp;
        if( !obj.IsUninitialized() )
            tmp = obj.ToString();

        ar & tmp;

        obj = Trigger( tmp );
    }

    // ------------------------------------------------------------------------
    // --- BaseEventTriggerFactory
    // ------------------------------------------------------------------------

    template<class Trigger, class Factory>
    BaseEventTriggerFactory<Trigger,Factory>::BaseEventTriggerFactory()
        : JsonConfigurable()
        , m_VectorUser()
        , m_VectorAll()
        , m_MapAll()
        , m_BuiltInNames()
    {
    }

    template<class Trigger, class Factory>
    BaseEventTriggerFactory<Trigger, Factory>::~BaseEventTriggerFactory()
    {
    }

    template<class Trigger, class Factory>
    bool BaseEventTriggerFactory<Trigger, Factory>::Configure( const Configuration* inputJson )
    {
        std::vector<std::string> user_events;
        initConfigTypeMap( USER_EVENTS_PARAMETER_NAME, &user_events, USER_EVENTS_PARAMETER_DESC );

        bool ret = JsonConfigurable::Configure( inputJson );

        // ---------------------------------------------------------------------------
        // --- NOTE: we want to execute the following code even if we ar egenerating
        // --- the schema so that we create the internal triggers.
        // ---------------------------------------------------------------------------
        if( ret )
        {
            if( ret && std::find( user_events.begin(), user_events.end(), "" ) != user_events.end() )
            {
                std::stringstream ss;
                ss << "Invalid Event in '" << USER_EVENTS_PARAMETER_NAME << "'.  Empty string is an invalid event.";
                throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            // ----------------------------------------------------
            // --- See the top of the file about Built-in triggers
            // ----------------------------------------------------
            bool create_built_in = ((m_VectorBuiltIn.size() > 0) && (m_VectorBuiltIn[ 0 ].second->m_pInternal == nullptr));
            if( create_built_in )
            {
                for( auto& entry : m_VectorBuiltIn )
                {
                    entry.second->m_pInternal = Add( entry.first );
                }
            }

            for( auto& ev : user_events )
            {
                CreateUserEventTrigger( ev );
            }
        }

        return ret;
    }

    template<class Trigger, class Factory>
    int BaseEventTriggerFactory<Trigger, Factory>::GetNumEventTriggers() const
    {
        return m_VectorAll.size();
    }

    template<class Trigger, class Factory>
    bool BaseEventTriggerFactory<Trigger, Factory>::IsValidEvent( const std::string& candidateEvent ) const
    {
        return (m_MapAll.count( candidateEvent ) > 0 );
    }

    template<class Trigger, class Factory>
    const std::string& BaseEventTriggerFactory<Trigger, Factory>::GetEventTriggerName( int eventIndex ) const
    {
        release_assert( eventIndex < m_VectorAll.size() );
        return m_VectorAll[ eventIndex ]->m_Name;
    }

    template<class Trigger, class Factory>
    EventTriggerInternal* BaseEventTriggerFactory<Trigger, Factory>::GetEventTriggerInternal( const std::string& str )
    {
        if( str.empty() )
        {
            return nullptr;
        }
        if( m_MapAll.count( str ) == 0 )
        {
            std::stringstream ss;
            ss << "Unknown event = '" << str << "'.\n";
            ss << "The known events are:\n";
            for( auto& entry : m_MapAll )
            {
                ss << entry.first << "\n";
            }
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        return m_MapAll.at( str );
    }

    template<class Trigger, class Factory>
    EventTriggerInternal* BaseEventTriggerFactory<Trigger, Factory>::GetEventTriggerInternal( const char* str )
    {
        return GetEventTriggerInternal( std::string( str ) );
    }

    template<class Trigger, class Factory>
    EventTriggerInternal* BaseEventTriggerFactory<Trigger, Factory>::Add( const std::string& str )
    {
        if( m_MapAll.count( str ) != 0 )
        {
            std::stringstream ss;
            if( std::find( GetBuiltInNames().begin(), GetBuiltInNames().end(), str ) != GetBuiltInNames().end() )
            {
                ss << "Duplicate event = '" << str << "'. This is a Built-in Event. You do not need to define it. The Built-in events are:" << std::endl;
                for( std::string name : GetBuiltInNames() )
                {
                    ss << name << std::endl;
                }
            }
            else
            {
                ss << "Duplicate event = '" << str << "'.  Events names must be unique.";
            }
            throw InvalidInputDataException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
        int index = m_VectorAll.size();
        EventTriggerInternal* p_eti = new EventTriggerInternal( str, index );
        m_VectorAll.push_back( p_eti );
        m_MapAll[ str ] = p_eti;
        return p_eti;
    }

    template<class Trigger, class Factory>
    std::vector<Trigger> BaseEventTriggerFactory<Trigger, Factory>::GetAllEventTriggers()
    {
        std::vector<Trigger> trigger_list;
        for( auto trigger : m_VectorAll )
        {
            trigger_list.push_back( Trigger( trigger ) );
        }
        return trigger_list;
    }

    template<class Trigger, class Factory>
    Trigger BaseEventTriggerFactory<Trigger, Factory>::CreateTrigger( const std::string& rParamName,
                                                                      const std::string& rTriggerName )
    {
        if( rTriggerName.empty() )
            return Trigger();
        
        if( !Factory::GetInstance()->IsValidEvent( rTriggerName ) )
        {
            std::ostringstream msg;
            msg << "'" << rParamName
                << "', a "
                << EventType::pairs::lookup_key( EVENT_TYPE )
                << " event type, with specified value '"
                << rTriggerName
                << "' is invalid.\nCustom "
                << EventType::pairs::lookup_key( EVENT_TYPE )
                << " events must be defined in the "
                << USER_EVENTS_PARAMETER_NAME
                << " list.\nThe built-in plus custom events are:\n";
            for( auto value : Factory::GetInstance()->GetAllEventTriggers() )
            {
                msg << value.ToString() << "...";
            }
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        return Trigger( rTriggerName );
    }

    template<class Trigger, class Factory>
    std::vector<Trigger> BaseEventTriggerFactory<Trigger, Factory>::CreateTriggerList( const std::string& rParamName,
                                                                                       const std::vector<std::string>& rTriggerNameList )
    {
        std::vector<Trigger> trigger_list;

        for( auto& name : rTriggerNameList )
        {
            if( name.empty() )
            {
                std::ostringstream msg;
                msg << "'" << rParamName
                    << "', a "
                    << EventType::pairs::lookup_key( EVENT_TYPE )
                    << " event type, with empty string is invalid.\n"
                    << "Custom "
                    << EventType::pairs::lookup_key( EVENT_TYPE )
                    << " events must be defined in the "
                    << USER_EVENTS_PARAMETER_NAME
                    << " list.\nThe built-in plus custom events are:\n";
                for( auto value : Factory::GetInstance()->GetAllEventTriggers() )
                {
                    msg << value.ToString() << "...";
                }
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
            trigger_list.push_back( CreateTrigger( rParamName, name ) );
        }

        return trigger_list;
    }

    template<class Trigger, class Factory>
    Trigger BaseEventTriggerFactory<Trigger, Factory>::CreateUserEventTrigger( const std::string& name )
    {
        EventTriggerInternal* p_eti = Add( std::string( name ) );
        m_VectorUser.push_back( p_eti );
        Trigger et( p_eti );
        return et;
    }

    template<class Trigger, class Factory>
    const std::vector<std::string>& BaseEventTriggerFactory<Trigger, Factory>::GetBuiltInNames()
    {
        if( m_BuiltInNames.size() == 0 )
        {
            for( auto entry : m_VectorBuiltIn )
            {
                m_BuiltInNames.push_back( entry.first );
            }
        }
        return m_BuiltInNames;
    }

    template<class Trigger, class Factory>
    const Trigger& BaseEventTriggerFactory<Trigger, Factory>::CreateBuiltInEventTrigger( const char* name )
    {
        // -----------------------------------------------------------------------------------
        // --- See the top of the file for more info, but notice that we do not create the
        // --- EventTriggerInternal, just the EventTrigger shell.  This allows us to update
        // --- the shell in the EXE when we create the internal or update the shell in the DLL
        // --- with the pointer in the EXE.
        // -----------------------------------------------------------------------------------
        std::pair<std::string, Trigger*> entry;
        entry.first = name;
        entry.second = new Trigger();
        m_VectorBuiltIn.push_back( entry );
        return *(m_VectorBuiltIn.back().second);
    }

    template<class Trigger, class Factory>
    void BaseEventTriggerFactory<Trigger, Factory>::SetBuiltIn()
    {
        // ----------------------------------------------------
        // --- See the top of the file about Built-in triggers.
        // --- Should only be called from DLLs and triggers should
        // --- not be used before hand.
        // ----------------------------------------------------

        Factory* p_factory = (Factory*)Environment::getEventTriggerFactory( EVENT_TYPE );
        release_assert( p_factory != nullptr );

        for( int i = 0; i < m_VectorBuiltIn.size(); ++i )
        {
            if( p_factory->m_MapAll.count( m_VectorBuiltIn[ i ].first ) == 0 )
            {
                std::stringstream ss;
                ss << "DLL Built-in trigger = " << m_VectorBuiltIn[ i ].first << " is missing in the EXE.";
                throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }

            m_VectorBuiltIn[ i ].second->m_pInternal = p_factory->m_MapAll.at( m_VectorBuiltIn[ i ].first );
        }
    }

    template<class Trigger, class Factory>
    void BaseEventTriggerFactory<Trigger, Factory>::DeleteInstance()
    {
        Factory* p_factory = (Factory*)Environment::getEventTriggerFactory( EVENT_TYPE );
        if( p_factory != nullptr )
        {
            // ---------------------------------------------------------------------------------
            // --- The Built-In events only get created once per execution.  For componentTests,
            // --- we need to get rid of the user defined events but maintain the built-in ones.
            // ---------------------------------------------------------------------------------
            p_factory->m_VectorAll.clear();
            p_factory->m_MapAll.clear();
            for( EventTriggerInternal* p_eti : p_factory->m_VectorUser )
            {
                delete p_eti;
            }
            p_factory->m_VectorUser.clear();
            delete p_factory;
        }
        Environment::setEventTriggerFactory( EVENT_TYPE, nullptr );
    }

    template<class Trigger, class Factory>
    Factory* BaseEventTriggerFactory<Trigger, Factory>::GetInstance()
    {
        Factory* p_factory = (Factory*)Environment::getEventTriggerFactory( EVENT_TYPE );
        if( p_factory == nullptr )
        {
            p_factory = new Factory();
            bool built_in_created = ((m_VectorBuiltIn.size() > 0) && (m_VectorBuiltIn[ 0 ].second->m_pInternal != nullptr));

            // ---------------------------------------------------------------------------------
            // --- The Built-In events only get created once per execution.  For componentTests,
            // --- we need to get rid of the user defined events but maintain the built-in ones.
            // ---------------------------------------------------------------------------------
            if( built_in_created )
            {
                for( auto& entry : m_VectorBuiltIn )
                {
                    p_factory->m_VectorAll.push_back( entry.second->m_pInternal );
                    p_factory->m_MapAll[ entry.first ] = entry.second->m_pInternal;
                }
            }
            Environment::setEventTriggerFactory( EVENT_TYPE, p_factory );
        }
        return p_factory;
    }
}
