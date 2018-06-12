/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "StandardEventCoordinator.h"

namespace Kernel
{
    class CoverageByNodeJson: public JsonConfigurable, public IComplexJsonConfigurable
    {
        public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

            CoverageByNodeJson() {}
            virtual void ConfigureFromJsonAndKey( const Configuration* inputJson, const std::string& key ) override;
            virtual json::QuickBuilder GetSchema() override;
            virtual bool  HasValidDefault() const override { return false; }
    public:
            std::map<uint32_t, float> node_coverage_map;
    };

    class CoverageByNodeEventCoordinator : public StandardInterventionDistributionEventCoordinator
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, CoverageByNodeEventCoordinator, IEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        CoverageByNodeEventCoordinator();
        virtual bool Configure( const Configuration * inputJson );

    protected:
        virtual bool TargetedIndividualIsCovered(IIndividualHumanEventContext *ihec);

        CoverageByNodeJson coverage_by_node;
    };
}
