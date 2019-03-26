/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVRandomChoice.h"

#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function
#include "IIndividualHumanContext.h"
#include "RANDOM.h"
#include <numeric>

SETUP_LOGGING( "HIVRandomChoice" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVRandomChoice, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVRandomChoice, HIVSimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(HIVRandomChoice)

    HIVRandomChoice::HIVRandomChoice()
    : HIVSimpleDiagnostic()
    {
        initSimTypes(1, "HIV_SIM" ); // just limiting this to HIV for release
    }

    bool HIVRandomChoice::Configure( const Configuration* inputJson )
    {
        std::vector<std::string> names;
        std::vector<float> values;

        initConfigTypeMap("Choice_Names", &names, HIV_Random_Choice_Names_DESC_TEXT);
        initConfigTypeMap("Choice_Probabilities", &values, HIV_Random_Choice_Probabilities_DESC_TEXT, 0.0f, 1.0f);

        bool val = HIVSimpleDiagnostic::Configure(inputJson);

        ProcessChoices(names, values);

        return val;
    }

    void HIVRandomChoice::ProcessChoices(std::vector<std::string> &names, std::vector<float> &values)
    {
        if (names.size() != values.size())
        {
            std::stringstream message;
            message << "Size of Choice_Names (" << names.size() << ") does not match size of Choice_Probabilities ("
                << values.size() << ") for intervention \"" << GetName() << "\"";
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, message.str().c_str());
        }

        float total = 0.0;


        for (int i = 0; i < names.size(); i++)
        {
            EventTrigger event = EventTriggerFactory::GetInstance()->CreateTrigger("Choices", names[i]);
            float probability = values[i];
            event_names.push_back(event);
            event_probabilities.push_back(probability);
            total += probability;
        }

        if ( total == 0.0 && !JsonConfigurable::_dryrun )
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "The sum of the probabilities in the 'Choices' table must be > 0.");
        }

        // Normalize the probabilities
        for (int i = 0; i < event_probabilities.size(); i++)
        {
            event_probabilities[i] = event_probabilities[i] / total;
        }
    }

    bool HIVRandomChoice::positiveTestResult()
    {
        return true;
    }

    void HIVRandomChoice::positiveTestDistribute()
    {
        LOG_DEBUG_F( "Individual %d tested HIVRandomChoice receiving actual intervention from HIVRandomChoice.\n", parent->GetSuid().data );

        // random number to choose an event from the dictionary
        float p = parent->GetRng()->e();

        EventTrigger trigger;
        float probSum = 0;

        // pick the EventTrigger to broadcast
        for (int i = 0; i < event_names.size(); i++)
        {
            probSum += event_probabilities[i];
            if (p <= probSum)
            {
                trigger = event_names[i];
                break;
            }
        }

        // expire the intervention
        expired = true;

        // broadcast the event
        if( !trigger.IsUninitialized() )
        {
            IIndividualEventBroadcaster* broadcaster = parent->GetEventContext()->GetNodeEventContext()->GetIndividualEventBroadcaster();
            broadcaster->TriggerObservers( parent->GetEventContext(), trigger );
        }
    }

    REGISTER_SERIALIZABLE(HIVRandomChoice);

    void HIVRandomChoice::serialize(IArchive& ar, HIVRandomChoice* obj)
    {
        HIVSimpleDiagnostic::serialize( ar, obj );
        HIVRandomChoice& choice = *obj;

        ar.labelElement("event_names") & choice.event_names;
        ar.labelElement("event_probabilities") & choice.event_probabilities;
    }
}
