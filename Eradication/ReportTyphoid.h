/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "ReportEnvironmental.h"
#include "TyphoidDefs.h" // for N_TYPHOID_SEROTYPES
#include "SimulationEnums.h" // for TyphoidVirusTypes
#include "TransmissionGroupMembership.h"
#include "ISimulationContext.h"
#include <map>

namespace Kernel {

    class ReportTyphoid : public ReportEnvironmental
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportTyphoid)
        public:
            ReportTyphoid();
            virtual ~ReportTyphoid() {};

            static IReport* ReportTyphoid::CreateReport() { return new ReportTyphoid(); }

            virtual bool Configure( const Configuration * inputJson );
            virtual void BeginTimestep() override;
            virtual void EndTimestep( float currentTime, float dt );
            virtual void AccumulateSEIRW();

            virtual void LogIndividualData( IIndividualHuman * individual);
            virtual void LogNodeData( Kernel::INodeContext * pNC );

        protected:
            virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );
            virtual void postProcessAccumulatedData();
            void setRecordingFlag();

        private:

            TransmissionGroupMembership_t memberships;
            ISimulationContext * parent;
            float startYear;
            float stopYear;
            bool recording;

            NonNegativeFloat chron_carriers_counter;
            NonNegativeFloat subclinical_infections_counter;
            NonNegativeFloat acute_infections_counter;
    };

}
