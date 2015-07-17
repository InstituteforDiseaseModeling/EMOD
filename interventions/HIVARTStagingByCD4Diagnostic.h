/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "InterpolatedValueMap.h"
#include "HIVARTStagingAbstract.h"

namespace Kernel
{
    class IDMAPI HIVARTStagingByCD4Diagnostic : public HIVARTStagingAbstract
    {
        DECLARE_FACTORY_REGISTERED(InterventionFactory, HIVARTStagingByCD4Diagnostic, IDistributableIntervention)

    public: 
        HIVARTStagingByCD4Diagnostic();
        HIVARTStagingByCD4Diagnostic( const HIVARTStagingByCD4Diagnostic& );

        // IDistributingDistributableIntervention
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject);

    protected:
        virtual bool positiveTestResult( IIndividualHumanHIV *pHIV, 
                                         float year, 
                                         float CD4count, 
                                         bool hasActiveTB, 
                                         bool isPregnant );

#pragma warning( push )
#pragma warning( disable: 4251 ) // See IdmApi.h for details
        InterpolatedValueMap threshold;
        InterpolatedValueMap ifActiveTB;
        InterpolatedValueMap ifPregnant;
#pragma warning( pop )

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, HIVARTStagingByCD4Diagnostic &obj, const unsigned int v);
#endif
    };
}
