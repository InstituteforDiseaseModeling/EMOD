/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "PropertyReport.h"
#include "IndividualCoInfection.h"

namespace Kernel {

    class PropertyReportTB : public PropertyReport
    {
    public:
        static IReport* CreateReport();
        virtual ~PropertyReportTB() { }

        virtual void LogIndividualData( Kernel::IIndividualHuman* individual ) override;

        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;


    protected:
        PropertyReportTB();

        virtual void postProcessAccumulatedData() override;

        // counters
        std::map< std::string, float > active_infections;
        std::map< std::string, float > active_naive_infections;
        std::map< std::string, float > active_retx_infections;
        std::map< std::string, float > active_naive_symptomatic_infections;

    //    std::map< std::string, float > pr_infections;
        std::map< std::string, float > active_smearpos_infections;

        std::map< std::string, float > disease_deaths_onTreatment;
        std::map< std::string, float > disease_deaths_txnaive;
        std::map< std::string, float > disease_deaths_MDR;
    //    std::map< std::string, float > disease_deaths_onTreatment_MDR;

        std::map< std::string, float > onTreatment;
    //    std::map< std::string, float > Naive_onTreatment;
    //    std::map< std::string, float > Retx_onTreatment;

        std::map< std::string, float > MDR_active_infections;
        std::map< std::string, float > MDR_active_evolved_infections;
        std::map< std::string, float > MDR_active_smearpos_infections;
        std::map< std::string, float > MDR_active_naive_infections;
        std::map< std::string, float > MDR_active_retx_infections;


        std::map< std::string, float > infectious_reservoir;
        std::map< std::string, float > presymp_tx_naive_infectious_reservoir;
        std::map< std::string, float > Retx_infectious_reservoir;
        std::map< std::string, float > active_symp_infectious_reservoir;
        std::map< std::string, float > active_symp_naive_infectious_reservoir;

        std::map< std::string, float > MDR_infectious_reservoir;
        std::map< std::string, float > MDR_presymp_tx_naive_infectious_reservoir;
        std::map< std::string, float > MDR_Retx_infectious_reservoir;
        std::map< std::string, float > MDR_active_symp_naive_infectious_reservoir;

        std::map< std::string, float > new_active_TB_infections;
    };

#if 0
    template<class Archive>
    void serialize(Archive &ar, PropertyReport& report, const unsigned int v)
    {
        boost::serialization::void_cast_register<PropertyReport,IReport>();
        //ar & report.timesteps_reduced;
        //ar & report.channelDataMap;
        //ar & report._nrmSize;
    }
#endif

}
