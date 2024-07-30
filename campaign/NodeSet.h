
#pragma once

#include <string>

#include "IdmApi.h"
#include "Configuration.h"
#include "ExternalNodeId.h"
#include "FactorySupport.h"
#include "Configure.h"
#include "InterventionEnums.h"
#include "ISerializable.h"
#include "ObjectFactory.h"

namespace Kernel
{
    ENUM_DEFINE(PolygonFormatType,
        ENUM_VALUE_SPEC(SHAPE      , 1) 
        //ENUM_VALUE_SPEC(GEOJSON    , 2)
        )

    struct INodeEventContext;

    struct IDMAPI INodeSet : public ISerializable
    {
        virtual bool Contains(INodeEventContext *ndc) = 0; // must provide access to demographics id, lat, long, etc
        virtual std::vector<ExternalNodeId_t> IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids) = 0;
    };

    class NodeSetFactory : public ObjectFactory<INodeSet,NodeSetFactory>
    {
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
        virtual std::vector<ExternalNodeId_t> IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids);

    protected:
        DECLARE_SERIALIZABLE(NodeSetAll);
    };

    class IDMAPI NodeSetPolygon : public INodeSet, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(NodeSetFactory, NodeSetPolygon, INodeSet)

    public:
        DECLARE_CONFIGURED(NodeSetPolygon)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        virtual bool Contains(INodeEventContext *ndc);
        virtual std::vector<ExternalNodeId_t> IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids);

    protected:
        void parseEmodFormat();
        void parseGeoJsonFormat();

        PolygonFormatType::Enum polygon_format;
        float * points_array;
        size_t num_points;
        std::string vertices_raw;

#if 0
    private:
        template<class Archive>
        void serialize(Archive &ar, NodeSetPolygon& nodeset, const unsigned int v)
        {
            ar & vertices_raw;
            ar & num_points;
            ar & polygon_format;
        }
#endif
    };

    class IDMAPI NodeListConfig : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        public:
            NodeListConfig() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
            virtual bool  HasValidDefault() const override { return false; }
            std::list< ExternalNodeId_t > nodelist;
    };

    class IDMAPI NodeSetNodeList : public INodeSet, public JsonConfigurable
    {
        DECLARE_FACTORY_REGISTERED(NodeSetFactory, NodeSetNodeList, INodeSet)

    public:
        DECLARE_CONFIGURED(NodeSetNodeList)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        virtual bool Contains(INodeEventContext *ndc);
        virtual std::vector<ExternalNodeId_t> IsSubset(const std::vector<ExternalNodeId_t>& demographic_node_ids);

    protected:
        NodeListConfig nodelist_config;
    };
};
