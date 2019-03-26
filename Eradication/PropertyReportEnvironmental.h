/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "PropertyReport.h"
#include "IndividualEnvironmental.h"

namespace Kernel {

    class PropertyReportEnvironmental : public PropertyReport
    {
        GET_SCHEMA_STATIC_WRAPPER( PropertyReportEnvironmental )
    public:
        static IReport* CreateReport();
        virtual ~PropertyReportEnvironmental() { }

        virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

    protected:
        PropertyReportEnvironmental();
        PropertyReportEnvironmental( const std::string& rReportName );

        // virtual void postProcessAccumulatedData() override;
        void reportContagionForRoute( const std::string& route, IndividualProperty* property, INodeContext* pNC );

        // counters
        std::map< std::string, float > new_contact_infections;
        std::map< std::string, float > new_enviro_infections;
    };
}
