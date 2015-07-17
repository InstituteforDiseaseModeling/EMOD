/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "SimpleTypemapRegistration.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "EventCoordinator.h"
#include "Configure.h"
#include "VectorEnums.h"
#include "VectorMatingStructure.h"

namespace Kernel
{
    class ResistanceHegGenetics : public JsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            ResistanceHegGenetics() {}
            virtual void ConfigureFromJsonAndKey( const Configuration *, const std::string &key );
            virtual json::QuickBuilder GetSchema();
            VectorAllele::Enum pesticideResistance;
            VectorAllele::Enum HEG;
    };

    struct IMosquitoRelease : public ISupports
    {
        virtual std::string GetSpecies() const = 0;
        virtual VectorMatingStructure GetVectorGenetics() const = 0;
        virtual int GetNumber() const = 0;
        virtual ~IMosquitoRelease() { }; // needed for cleanup via interface pointer
    };

    class MosquitoRelease : public IMosquitoRelease, public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(MosquitoRelease)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, MosquitoRelease, INodeDistributableIntervention)

    public:
        MosquitoRelease();
        virtual ~MosquitoRelease() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC);
        virtual void SetContextTo(INodeEventContext *context) { } // not needed for this intervention
        virtual void Update(float dt);

        // IMosquitoRelease
        virtual std::string GetSpecies() const;
        virtual VectorMatingStructure GetVectorGenetics() const;
        virtual int GetNumber() const;

    protected:
        JsonConfigurable::ConstrainedString releasedSpecies;
        VectorMatingStructure vector_genetics;
        ResistanceHegGenetics self;
        ResistanceHegGenetics mate;
        int releasedNumber;

        void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
    private:
#if USE_BOOST_SERIALIZATION
        friend class ::boost::serialization::access;

        template<class Archive>
        void serialize_inner(Archive &ar, const unsigned int v);
#endif
    };
}
