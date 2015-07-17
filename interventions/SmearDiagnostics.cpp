/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "SmearDiagnostics.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "TBContexts.h"

static const char * _module = "SmearDiagnostic";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(SmearDiagnostic, SimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(SmearDiagnostic, SimpleDiagnostic)

    IMPLEMENT_FACTORY_REGISTERED(SmearDiagnostic)

    bool SmearDiagnostic::Configure(
        const Configuration * inputJson
    )
    {
        return SimpleDiagnostic::Configure( inputJson );
    }

    SmearDiagnostic::SmearDiagnostic() : SimpleDiagnostic()
    {
        initSimTypes( 1, "TB_SIM" );
    }

    SmearDiagnostic::SmearDiagnostic( const SmearDiagnostic& master )
    : SimpleDiagnostic( master )
    {
    }

    SmearDiagnostic::~SmearDiagnostic()
    { 
        LOG_DEBUG("Destructing Smear Diagnostic \n");
    }

    bool SmearDiagnostic::positiveTestResult()
    {
        LOG_DEBUG("Positive test Result function\n");

        // Apply diagnostic test with given specificity/sensitivity
        float rand = parent->GetRng()->e();

        IIndividualHumanTB* tb_ind = NULL;
        if(parent->QueryInterface( GET_IID( IIndividualHumanTB ), (void**)&tb_ind ) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanTB", "IIndividualHuman" );
        }
        bool activeinf = tb_ind->HasActiveInfection() && !tb_ind->HasActivePresymptomaticInfection();
        bool smearpos = tb_ind->IsSmearPositive();

        // always return negative if the person is not infected, intended to be used with GroupEventCoordinator
        // TODO: allow to distribute Smear diagnostic to non-infected individuals?

        if (activeinf)
        {
            // True positive (sensitivity), or False positive (1-specificity)
            bool positiveTest = ( smearpos && (rand < base_sensitivity) ) || ( !smearpos && (rand > base_specificity) );
            return positiveTest;
        }
        else
        { return false;}
    }


}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::SmearDiagnostic)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, SmearDiagnostic& obj, const unsigned int v)
    {

        boost::serialization::void_cast_register<SmearDiagnostic, IDistributableIntervention>();

        ar & boost::serialization::base_object<Kernel::SimpleDiagnostic>(obj);
    }
}
#endif
