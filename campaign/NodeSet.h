/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>

#include "IdmApi.h"
#include "BoostLibWrapper.h"
#include "ISupports.h"
#include "Configuration.h"
#include "Contexts.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "InterventionEnums.h"

namespace Kernel
{
    struct INodeEventContext;

    struct IDMAPI INodeSet : public ISupports
    {
        virtual bool Contains(INodeEventContext *ndc) = 0; // must provide access to demographics id, lat, long, etc
    };

    class IDMAPI INodeSetFactory
    {
    public:
        virtual void Register(string classname, instantiator_function_t _if) = 0;
        virtual json::QuickBuilder GetSchema() = 0;
    };            

    class IDMAPI NodeSetFactory : public INodeSetFactory
    {
    public:
        static INodeSetFactory * getInstance() { return _instance ? _instance : _instance = new NodeSetFactory(); }

        static INodeSet* CreateInstance(const Configuration * config)
        {
            return CreateInstanceFromSpecs<INodeSet>(config, getRegisteredClasses(), true);
        }
        void Register(string classname, instantiator_function_t _if)  {  getRegisteredClasses()[classname] = _if;  }
        json::QuickBuilder GetSchema();

    protected:
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        static json::Object campaignSchema;
        static support_spec_map_t& getRegisteredClasses() { static support_spec_map_t registered_classes; return registered_classes; }
#pragma warning( pop )

    private:
        static INodeSetFactory * _instance;
    };


    // class defines a simple set of nodes...either by id, 
    class IDMAPI NodeSetAll : public INodeSet, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(NodeSetFactory, NodeSetAll, INodeSet)

    public:
        DECLARE_CONFIGURED(NodeSetAll)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        
        virtual bool Contains(INodeEventContext *ndc);

#if USE_JSON_SERIALIZATION
    public:

        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        void serialize(Archive &ar, NodeSetAll& nodeset, const unsigned int v)
        {
            boost::serialization::void_cast_register<NodeSetAll, INodeSet>();
        }
#endif
    };

    class IDMAPI NodeSetPolygon : public INodeSet, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(NodeSetFactory, NodeSetPolygon, INodeSet)

    public:
        DECLARE_CONFIGURED(NodeSetPolygon)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        virtual bool Contains(INodeEventContext *ndc);

    protected:
        void parseEmodFormat();
        void parseGeoJsonFormat();

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        PolygonFormatType::Enum polygon_format;
        float * points_array;
        size_t num_points;
        std::string vertices_raw;
#pragma warning( pop )

#if USE_JSON_SERIALIZATION
    public:

        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        void serialize(Archive &ar, NodeSetPolygon& nodeset, const unsigned int v)
        {
            boost::serialization::void_cast_register<NodeSetPolygon, INodeSet>();
            ar & vertices_raw;
            ar & num_points;
            ar & polygon_format;
        }
#endif
    };

    class IDMAPI NodeListConfig : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        public:
            NodeListConfig() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key );
            virtual json::QuickBuilder GetSchema();
#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
            tNodeIdList nodelist;
#pragma warning( pop )
    };

    class IDMAPI NodeSetNodeList : public INodeSet, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED(NodeSetFactory, NodeSetNodeList, INodeSet)

    public:
        DECLARE_CONFIGURED(NodeSetNodeList)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        virtual bool Contains(INodeEventContext *ndc);

    protected:
        NodeListConfig nodelist_config;

#if USE_JSON_SERIALIZATION
    public:

        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, NodeSetNodeList& nodeset, const unsigned int v);
#endif
    };
};
