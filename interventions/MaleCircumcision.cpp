/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "MaleCircumcision.h"

#include "Contexts.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"

#include "STIInterventionsContainer.h"

static const char * _module = "MaleCircumcision";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MaleCircumcision)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(ICircumcision)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MaleCircumcision)

    IMPLEMENT_FACTORY_REGISTERED(MaleCircumcision)
    
    MaleCircumcision::MaleCircumcision()
    {
        initSimTypes( 2, "STI_SIM", "HIV_SIM" );
    }

    bool
    MaleCircumcision::Configure(
        const Configuration * inputJson
    )
    {
        return JsonConfigurable::Configure( inputJson );
    }

    void MaleCircumcision::Update( float dt )
    {
        // Nothing to do for this intervention, which doesn't have ongoing effects
    }

    void MaleCircumcision::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        // Nothing to do for this intervention
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::MaleCircumcision)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, MaleCircumcision& obj, const unsigned int v)
    {
        boost::serialization::void_cast_register<MaleCircumcision, IDistributableIntervention>();
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(obj);
    }
}
#endif