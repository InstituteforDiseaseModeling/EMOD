/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <math.h> // for std::floor
#include "Exceptions.h"
#include "HIVPiecewiseByYearAndSexDiagnostic.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IIndividualHumanHIV.h"
#include "SusceptibilityHIV.h"
#include "IHIVInterventionsContainer.h"
#include "SimulationEnums.h"

static const char * _module = "HIVPiecewiseByYearAndSexDiagnostic";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(HIVPiecewiseByYearAndSexDiagnostic)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        //HANDLE_INTERFACE(IHealthSeekingBehavior)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(HIVPiecewiseByYearAndSexDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVPiecewiseByYearAndSexDiagnostic)

    HIVPiecewiseByYearAndSexDiagnostic::HIVPiecewiseByYearAndSexDiagnostic()
    : HIVSimpleDiagnostic()
    , interpolation_order(0)
    , female_multiplier(1)
    , default_value(0)
    , period_between_trials(0)
    {
        initConfigTypeMap("Interpolation_Order", &interpolation_order, HIV_PBYASD_Interpolation_Order_DESC_TEXT, 0, 1, 0);
        initConfigTypeMap("Female_Multiplier", &female_multiplier, HIV_PBYASD_Female_Multiplier_DESC_TEXT, 0, FLT_MAX, 1);
        initConfigTypeMap("Default_Value", &default_value, HIV_PBYASD_Default_Value_DESC_TEXT, 0, 1, 0);
        initConfigComplexType("Time_Value_Map", &year2ValueMap, HIV_PBYASD_Time_Value_Map_DESC_TEXT);
    };

    HIVPiecewiseByYearAndSexDiagnostic::HIVPiecewiseByYearAndSexDiagnostic( const HIVPiecewiseByYearAndSexDiagnostic& master )
    : HIVSimpleDiagnostic( master )
    {
        interpolation_order = master.interpolation_order;
        female_multiplier = master.female_multiplier;
        default_value = master.default_value;
        period_between_trials = master.period_between_trials;
        year2ValueMap = master.year2ValueMap;
        period_between_trials = master.period_between_trials;
        value_multiplier = master.value_multiplier;
    }

    bool
    HIVPiecewiseByYearAndSexDiagnostic::Configure(const Configuration* inputJson)
    {
        bool ret = HIVSimpleDiagnostic::Configure(inputJson);
        value_multiplier = period_between_trials/DAYSPERYEAR;   // Will use first order approximation of exponential
        return ret;
    }

    bool
    HIVPiecewiseByYearAndSexDiagnostic::positiveTestResult()
    {
        bool testResult = 0;
        float value = 0;

        float year = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
        auto gender = parent->GetEventContext()->GetGender();

        switch (interpolation_order)
        {
        case 0:
            value = year2ValueMap.getValuePiecewiseConstant(year, default_value);
            break;
        case 1:
            value = year2ValueMap.getValueLinearInterpolation(year, default_value);
            break;
        default:
            LOG_WARN_F("Invalid selection for interpolation order.  Using linear interpolation as default.");
            value = year2ValueMap.getValueLinearInterpolation(year, default_value);
        }

        value = (gender == Gender::FEMALE) ? value * female_multiplier : value;
        testResult = (randgen->e() < value);

        LOG_DEBUG_F("Individual %d: sex=%d, year=%f, returned %d.\n", parent->GetSuid().data, gender, year, testResult);

        return testResult;
    }

    REGISTER_SERIALIZABLE(HIVPiecewiseByYearAndSexDiagnostic);

    void HIVPiecewiseByYearAndSexDiagnostic::serialize(IArchive& ar, HIVPiecewiseByYearAndSexDiagnostic* obj)
    {
        HIVSimpleDiagnostic::serialize( ar, obj );
        HIVPiecewiseByYearAndSexDiagnostic& diag = *obj;

        ar.labelElement("interpolation_order"  ) & diag.interpolation_order;
        ar.labelElement("female_multiplier"    ) & diag.female_multiplier;
        ar.labelElement("default_value"        ) & diag.default_value;
        ar.labelElement("year2ValueMap"        ) & diag.year2ValueMap;
        ar.labelElement("period_between_trials") & diag.period_between_trials;
        ar.labelElement("value_multiplier"     ) & diag.value_multiplier;
    }
}
