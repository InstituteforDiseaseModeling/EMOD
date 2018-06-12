/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "Interventions.h"
#include "Configuration.h"
#include "InterventionFactory.h"
#include "InterventionEnums.h"
#include "EventCoordinator.h"
#include "Configure.h"
#include "VectorEnums.h"
#include "VectorMatingStructure.h"

namespace Kernel
{
    class ResistanceHegGenetics : public JsonConfigurable, public IComplexJsonConfigurable
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        public:
            ResistanceHegGenetics() {}
            virtual void ConfigureFromJsonAndKey( const Configuration *, const std::string &key ) override;
            virtual json::QuickBuilder GetSchema() override;
            virtual bool  HasValidDefault() const override { return false; }
            VectorAllele::Enum pesticideResistance;
            VectorAllele::Enum HEG;
    };

    class MosquitoRelease : public BaseNodeIntervention
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_CONFIGURED(MosquitoRelease)
        DECLARE_FACTORY_REGISTERED(InterventionFactory, MosquitoRelease, INodeDistributableIntervention)

    public:
        MosquitoRelease();
        MosquitoRelease( const MosquitoRelease& master );
        virtual ~MosquitoRelease() { }

        // INodeDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);
        virtual bool Distribute(INodeEventContext *context, IEventCoordinator2* pEC);
        virtual void SetContextTo(INodeEventContext *context) { } // not needed for this intervention
        virtual void Update(float dt);

    protected:
        virtual const std::string& getSpecies() const;
        virtual const VectorMatingStructure& getVectorGenetics() const;
        virtual uint32_t getNumber() const;

    protected:
        jsonConfigurable::ConstrainedString releasedSpecies;
        VectorMatingStructure vector_genetics;
        ResistanceHegGenetics self;
        ResistanceHegGenetics mate;
        uint32_t releasedNumber;
    };
}
