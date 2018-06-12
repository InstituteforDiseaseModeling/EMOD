/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BinnedReportTB.h"

#include "Environment.h"
#include "Sugar.h"
#include "IndividualCoInfection.h"

using namespace std;
using namespace json;

SETUP_LOGGING( "BinnedReportTB" )

namespace Kernel {

    static const char * _active_cases_label = "Active Cases";
    static const char * _active_smearpos_label = "Active Smearpos Cases";
    static const char * _active_mdr_label = "MDR Active Cases";
    static const char * _infectiousness_label = "Infectiousness";
    static const char * _tx_naive_infectiousness_label = "Tx Naive Infectiousness";
    static const char * _infectiousness_from_fast_label = "Infectiousness from Fast";
    static const char * _MDR_infectiousness_label = "MDR Infectiousness";
    static const char * _active_from_fast_label = "Active From Fast Cases";
    static const char * _MDR_active_from_fast_infection_label = "MDR Active From Fast Cases";
    static const char * _disease_deaths_ontx_label = "Deaths on Tx";

    IReport*
    BinnedReportTB::CreateReport()
    {
        return new BinnedReportTB();
    }

    // Derived constructor calls base constructor to initialized reduced timesteps etc. 
    BinnedReportTB::BinnedReportTB() 
        : BinnedReport()
        , active_infection_bins(nullptr)
        , MDR_active_infections_bins(nullptr)
        , infectiousness_bins(nullptr)
        , Tx_naive_infectiousness_bins(nullptr)
        , infectiousness_from_fast_bins(nullptr)
        , MDR_infectiousness_bins(nullptr)
        , active_from_fast_infection_bins(nullptr)
        , MDR_active_from_fast_infection_bins(nullptr)
        , disease_deaths_ontx_bins(nullptr)
        , active_smearpos_infection_bins(nullptr)
    {
        LOG_DEBUG( "BinnedReportTB ctor\n" );
    }

    BinnedReportTB::~BinnedReportTB()
    {
        delete[] active_infection_bins;
        delete[] MDR_active_infections_bins;
        delete[] infectiousness_bins;
        delete[] Tx_naive_infectiousness_bins;
        delete[] infectiousness_from_fast_bins;
        delete[] MDR_infectiousness_bins;
        delete[] active_from_fast_infection_bins;
        delete[] MDR_active_from_fast_infection_bins;
        delete[] disease_deaths_ontx_bins;
        delete[] active_smearpos_infection_bins;
    }

    void BinnedReportTB::initChannelBins()
    {
        BinnedReport::initChannelBins();

        active_infection_bins = new float[num_total_bins];
        MDR_active_infections_bins = new float[num_total_bins];
        infectiousness_bins = new float[num_total_bins];
        Tx_naive_infectiousness_bins = new float[num_total_bins];
        infectiousness_from_fast_bins = new float[num_total_bins];
        MDR_infectiousness_bins = new float[num_total_bins];
        active_from_fast_infection_bins = new float[num_total_bins];
        MDR_active_from_fast_infection_bins = new float[num_total_bins];
        disease_deaths_ontx_bins = new float[num_total_bins];
        active_smearpos_infection_bins = new float[ num_total_bins];

        clearChannelsBins();
    }

    void BinnedReportTB::clearChannelsBins()
    {
        //don't need to call the base class since the clearChannelsBins in the base class is called from initChannelBins in the base class
        memset(active_infection_bins, 0, num_total_bins * sizeof(float));
        memset(MDR_active_infections_bins, 0, num_total_bins * sizeof(float));
        memset(infectiousness_bins, 0, num_total_bins * sizeof(float));
        memset(Tx_naive_infectiousness_bins, 0, num_total_bins * sizeof(float));
        memset(infectiousness_from_fast_bins, 0, num_total_bins * sizeof(float));
        memset(MDR_infectiousness_bins, 0, num_total_bins * sizeof(float));
        memset(active_from_fast_infection_bins, 0, num_total_bins * sizeof(float));
        memset(MDR_active_from_fast_infection_bins, 0, num_total_bins * sizeof(float));
        memset(disease_deaths_ontx_bins, 0, num_total_bins * sizeof(float));
        memset(active_smearpos_infection_bins, 0, num_total_bins * sizeof(float));
    }

    void BinnedReportTB::EndTimestep( float currentTime, float dt )
    {
        Accumulate(_active_cases_label, active_infection_bins);
        Accumulate(_active_mdr_label, MDR_active_infections_bins);
        Accumulate(_infectiousness_label, infectiousness_bins);
        Accumulate(_tx_naive_infectiousness_label, Tx_naive_infectiousness_bins);
        Accumulate(_infectiousness_from_fast_label, infectiousness_from_fast_bins);
        Accumulate(_MDR_infectiousness_label, MDR_infectiousness_bins);
        Accumulate(_active_from_fast_label, active_from_fast_infection_bins);
        Accumulate(_MDR_active_from_fast_infection_label, MDR_active_from_fast_infection_bins);
        Accumulate(_disease_deaths_ontx_label, disease_deaths_ontx_bins);
        Accumulate(_active_smearpos_label, active_smearpos_infection_bins);
        clearChannelsBins(); //this function calls the clear channelbins in binnedreportTB, then the base class calls its own clearChannelBins

        BinnedReport::EndTimestep( currentTime, dt );
    }

    void BinnedReportTB::LogIndividualData( Kernel::IIndividualHuman* individual )
    {
        LOG_DEBUG( "BinnedReportTB::LogIndividualData\n" );

        BinnedReport::LogIndividualData(individual);

        // Get individual weight and bin variables
        float mc_weight    = float(individual->GetMonteCarloWeight());
        int bin_index = calcBinIndex(individual);

        const Kernel::IndividualHumanCoInfection* individual_tb = static_cast<const Kernel::IndividualHumanCoInfection*>(individual);

        if(individual->GetStateChange() == HumanStateChange::KilledByInfection && individual_tb->IsOnTreatment())
        {
            disease_deaths_ontx_bins[bin_index] += mc_weight;
        }

        if (individual->IsInfected())
        {
            //infectiousness stuff
            infectiousness_bins[bin_index]     += (individual->GetInfectiousness() * mc_weight);
        
            if (individual_tb->IsTreatmentNaive())
            {
                Tx_naive_infectiousness_bins[bin_index] += (individual->GetInfectiousness() * mc_weight);
            }

            if (individual_tb->IsFastProgressor())
            {
                infectiousness_from_fast_bins[bin_index] += (individual->GetInfectiousness() * mc_weight);
            }
        
            if (individual_tb->IsMDR())
            {
                MDR_infectiousness_bins[bin_index] += (individual->GetInfectiousness() * mc_weight);
            }

            //static specific stuff
            if (individual_tb->HasActiveInfection())
            {
                active_infection_bins[bin_index]     += mc_weight;
                if (individual_tb->IsMDR()) 
                {
                    MDR_active_infections_bins[ bin_index ] += mc_weight;
                    if (individual_tb->IsFastProgressor())
                    {
                        MDR_active_from_fast_infection_bins[bin_index] += mc_weight;
                    }
                }

                if (individual_tb->IsFastProgressor())
                {
                    active_from_fast_infection_bins[bin_index] += mc_weight;
                }

                if ( ! individual_tb->HasActivePresymptomaticInfection() )
                {
                    if (individual_tb->IsSmearPositive())
                    {
                        active_smearpos_infection_bins[ bin_index] += mc_weight;
                    }
                }
            }
        }
    }

    void BinnedReportTB::postProcessAccumulatedData()
    {
        BinnedReport::postProcessAccumulatedData();
    }
}
