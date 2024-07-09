
#include "stdafx.h"
#include "ARTMortalityTable.h"
#include "IIndividualHumanHIV.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "IHIVInterventionsContainer.h"
#include "ISusceptibilityHIV.h"
#include "RANDOM.h"
#include "ReportUtilities.h"

SETUP_LOGGING("ARTMortalityTable")

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(ARTMortalityTable)

    ARTMortalityTable::ARTMortalityTable()
        : AntiretroviralTherapy()
        , m_art_duration_days_bins()
        , m_age_years_bins()
        , m_cd4_count_bins()
        , m_mortality_table()
    {
    }

    ARTMortalityTable::ARTMortalityTable(const ARTMortalityTable& rMaster)
        : AntiretroviralTherapy(rMaster)
        , m_art_duration_days_bins(rMaster.m_art_duration_days_bins)
        , m_age_years_bins(rMaster.m_age_years_bins)
        , m_cd4_count_bins(rMaster.m_cd4_count_bins)
        , m_mortality_table(rMaster.m_mortality_table)
    {
    }

    ARTMortalityTable::~ARTMortalityTable()
    {
    }

    void ARTMortalityTable::ConfigureMortalityParams(const Configuration * inputJson)
    {
        initConfigTypeMap("ART_Duration_Days_Bins", &m_art_duration_days_bins, ART_Duration_Days_Bins_DESC_TEXT, 0, FLT_MAX, true);
        initConfigTypeMap("Age_Years_Bins", &m_age_years_bins, ART_Age_Years_Bins_DESC_TEXT, 0.0f, MAX_HUMAN_AGE, true);
        initConfigTypeMap("CD4_Count_Bins", &m_cd4_count_bins, ART_CD4_Count_Bins_DESC_TEXT, 0.0f, 1000.0f, true);
        initConfigTypeMap("MortalityTable", &m_mortality_table, ART_Mortality_Table_DESC_TEXT, 0.0f, 1.0f);
    }

    void ARTMortalityTable::ValidateParams()
    {
        if (m_art_duration_days_bins.size() == 0 ||
            m_age_years_bins.size() == 0 ||
            m_cd4_count_bins.size() == 0)
        {
            throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, "Bins for indexing into 'MortalityTable' field of 'ARTMortalityTable' intervention must not be empty.");
        }

        bool anyMisshapen = false;

        anyMisshapen |= m_mortality_table.size() != m_art_duration_days_bins.size();
        for (auto& firstDim : m_mortality_table)
        {
            anyMisshapen |= firstDim.size() != m_age_years_bins.size();
            for (auto& secondDim : firstDim)
            {
                anyMisshapen |= secondDim.size() != m_cd4_count_bins.size();
            }
        }

        if (anyMisshapen)
        {
            throw InvalidInputDataException(__FILE__, __LINE__, __FUNCTION__, "MortalityTable dimensions do not match provided bins");
        }
    }

    bool ARTMortalityTable::CanDistribute(IIndividualHumanHIV* p_hiv_human)
    {
        return true;
    }
    
    float ARTMortalityTable::GetInterpolatedValue(
        float value, 
        const std::vector<float>& indexList,
        const std::vector<float>& valueList) const
    {
        int index = ReportUtilities::GetBinIndex(value, indexList);

        float interp = 0;

        if (value >= indexList.back())
        {
            interp = valueList.back();
        }
        else if (index == 0)
        {
            interp = valueList.front();
        }
        else {
            // The % of the way between the two indices
            interp = (value - indexList[index - 1]) / (indexList[index] - indexList[index - 1]);

            // That % translated into value space from index space
            interp *= valueList[index] - valueList[index - 1];

            // Translate that value to the right index
            interp += valueList[index - 1];
        }

        return interp;
    }

    float ARTMortalityTable::ComputeDurationFromEnrollmentToArtAidsDeath(IIndividualHumanContext* pPersonGoingOnArt,
        IIndividualHumanHIV* pPersonGoingOnArtHIV) const
    {
        IHIVInterventionsContainer* itbda = pPersonGoingOnArtHIV->GetHIVInterventionsContainer();
        IIndividualHuman* person = dynamic_cast<IIndividualHuman*>(pPersonGoingOnArtHIV);

        float durationOnArt = itbda->GetDurationSinceLastStartingART();
        float age_years = person->GetAge() / DAYSPERYEAR;
        float cd4Count = pPersonGoingOnArtHIV->GetHIVSusceptibility()->GetCD4count();

        int artDurationIndex = ReportUtilities::GetBinIndex(durationOnArt, m_art_duration_days_bins);
        int ageIndex = ReportUtilities::GetBinIndex(age_years, m_age_years_bins);

        float daysUntilDeath = FLT_MAX;
        do
        {
            float mortalityRate = GetInterpolatedValue(cd4Count, m_cd4_count_bins, m_mortality_table[artDurationIndex][ageIndex]);
            if (mortalityRate == 0.0f)
            {
                daysUntilDeath = FLT_MAX;
            }
            else
            {
                daysUntilDeath = pPersonGoingOnArt->GetRng()->expdist(mortalityRate) * DAYSPERYEAR;
            }

            artDurationIndex += 1;
        } while (artDurationIndex < m_art_duration_days_bins.size() &&
            daysUntilDeath > m_art_duration_days_bins[artDurationIndex - 1] - durationOnArt);

        return daysUntilDeath;
    }

    REGISTER_SERIALIZABLE(ARTMortalityTable);

    void ARTMortalityTable::serialize(IArchive& ar, ARTMortalityTable* obj)
    {
        vector<float> m_art_duration_days_bins;
        vector<float> m_age_years_bins;
        vector<float> m_cd4_count_bins;
        vector<vector<vector<float>>> m_mortality_table;

        AntiretroviralTherapy::serialize( ar, obj );
        ARTMortalityTable& art = *obj;
        ar.labelElement( "m_art_duration_days_bins" ) & art.m_art_duration_days_bins;
        ar.labelElement( "m_age_years_bins"         ) & art.m_age_years_bins;
        ar.labelElement( "m_cd4_count_bins"         ) & art.m_cd4_count_bins;
        ar.labelElement( "m_mortality_table"        ) & art.m_mortality_table;
    }
}