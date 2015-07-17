/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "BaseTextReport.h"
#include "ISimulation.h"
#include "IIndividualHuman.h"

namespace Kernel
{
    struct StiTransmissionInfo
    {
        NonNegativeFloat time;
        NonNegativeFloat year;

        unsigned long rel_id; // relationship id
        unsigned long source_id;
        bool source_is_infected;
        unsigned int source_gender;
        float source_age;
        unsigned int source_current_relationship_count;
        unsigned int source_lifetime_relationship_count;
        unsigned int source_relationships_in_last_6_months;
        unsigned int source_extrarelational_flags;
        unsigned int source_is_circumcised;
        bool source_has_sti;
        bool source_is_superspreader;
        float source_infection_age;
        unsigned long destination_id;
        bool destination_is_infected;
        unsigned int destination_gender;
        float destination_age;
        unsigned int destination_current_relationship_count;
        unsigned int destination_lifetime_relationship_count;
        unsigned int destination_relationships_in_last_6_months;
        unsigned int destination_extrarelational_flags;
        unsigned int destination_is_circumcised;
        bool destination_has_sti;
        bool destination_is_superspreader;
    };

    struct IIndividualHumanSTI;

    class StiTransmissionReporter : public BaseTextReport
    {
    public:
        static IReport* Create(ISimulation* simulation);

        // IReport
        virtual void BeginTimestep();
        virtual void EndTimestep( float currentTime, float dt );

    protected:
        StiTransmissionReporter(ISimulation* simulation);
        virtual ~StiTransmissionReporter();

        // BaseTextReport
        virtual std::string GetHeader() const ;

        void onNewNode(Kernel::INodeContext* node);
        void onTransmission(IIndividualHuman* individual);

        // Methods for subclasses to override so that they can add data
        virtual void ClearData();
        virtual void CollectOtherData( unsigned int relationshipID,
                                       IIndividualHumanSTI* pPartnerA,
                                       IIndividualHumanSTI* pPartnerB ) {} ;
        virtual std::string GetOtherData( unsigned int relationshipID ) { return ""; };

        // member variables
        ISimulation* simulation;
        std::vector<StiTransmissionInfo> report_data;
    };
}