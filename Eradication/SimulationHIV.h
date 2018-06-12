/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once
#include "SimulationSTI.h"
#include "Sugar.h" // for DECLARE_VIRTUAL_BASE
//#include "InterventionValidator.h"
#include "ChannelDataMap.h"

namespace Kernel
{
    class IndividualHumanHIV;

    class SimulationHIV : public SimulationSTI,
                          public IChannelDataMapOutputAugmentor
    {
        GET_SCHEMA_STATIC_WRAPPER(SimulationHIV)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        virtual ~SimulationHIV(void);
        static SimulationHIV *CreateSimulation();
        static SimulationHIV *CreateSimulation(const ::Configuration *config);
        SimulationHIV();
        virtual bool Configure( const ::Configuration *json );

        virtual void Reports_CreateBuiltIn();

        // IChannelDataMapOutputAugmentor
        virtual void AddDataToHeader( IJsonObjectAdapter* pIJsonObj );

    protected:
        virtual void Initialize() override;
        virtual void Initialize(const ::Configuration *config) override;
        static bool ValidateConfiguration(const ::Configuration *config);

        // Allows correct type of Node to be added by classes derived from Simulation
        virtual void addNewNodeFromDemographics( suids::suid node_suid,
                                                 NodeDemographicsFactory *nodedemographics_factory,
                                                 ClimateFactory *climate_factory,
                                                 bool white_list_enabled ) override;

        bool report_hiv_by_age_and_gender;
        bool report_hiv_ART;
        bool report_hiv_infection;
        bool report_hiv_mortality;
        float report_hiv_period;
    };
}
