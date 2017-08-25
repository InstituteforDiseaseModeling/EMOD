/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MalariaDiagnostic.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "MalariaContexts.h"

SETUP_LOGGING( "MalariaDiagnostic" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(MalariaDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(MalariaDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(MalariaDiagnostic)

    bool MalariaDiagnostic::Configure(const Configuration * inputJson)
    {
        initConfig( "Diagnostic_Type", malaria_diagnostic_type, inputJson, MetadataDescriptor::Enum("Diagnostic_Type", MD_Diagnostic_Type_DESC_TEXT, MDD_ENUM_ARGS( MalariaDiagnosticType ) ) );
        if( malaria_diagnostic_type == MalariaDiagnosticType::Other || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap("Detection_Threshold", &detection_threshold, MD_Detection_Threshold_DESC_TEXT, 0, 1e6, 100, "Diagnostic_Type", "Other" );
        }
        return SimpleDiagnostic::Configure(inputJson);
    }

    MalariaDiagnostic::MalariaDiagnostic()
    : SimpleDiagnostic()
    , malaria_diagnostic_type(MalariaDiagnosticType::Other)
    , detection_threshold(0)
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    MalariaDiagnostic::MalariaDiagnostic( const MalariaDiagnostic& master )
    : SimpleDiagnostic( master )
    {
        malaria_diagnostic_type = master.malaria_diagnostic_type;
        detection_threshold = master.detection_threshold;
    }

    MalariaDiagnostic::~MalariaDiagnostic()
    { 
        LOG_DEBUG("Destructing Malaria Diagnostic \n");
    }

    bool MalariaDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test Result function\n");

        IMalariaHumanContext* individual_malaria = NULL;
        if(parent->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&individual_malaria ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IMalariaHumanContext", "IIndividualHuman" );
        }

        if( malaria_diagnostic_type == MalariaDiagnosticType::Other)
        {
            float density = individual_malaria->GetMalariaSusceptibilityContext()->get_parasite_density();
            return density > detection_threshold;
        }
        else
        {
            bool is_microscopy = (malaria_diagnostic_type == MalariaDiagnosticType::Microscopy);
            int test = is_microscopy ? MALARIA_TEST_BLOOD_SMEAR : MALARIA_TEST_NEW_DIAGNOSTIC;
            return individual_malaria->CheckForParasitesWithTest( test );
        }
    }

    REGISTER_SERIALIZABLE(MalariaDiagnostic);

    void MalariaDiagnostic::serialize(IArchive& ar, MalariaDiagnostic* obj)
    {
        MalariaDiagnostic& md = *obj;
        ar.labelElement("malaria_diagnostic_type") & (uint32_t&)md.malaria_diagnostic_type;
        ar.labelElement("detection_threshold"    ) & md.detection_threshold;
    }
}
