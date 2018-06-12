/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "StiTransmissionReporter.h"

namespace Kernel
{
    class HIVTransmissionReporter : public StiTransmissionReporter
    {
    public:
        static IReport* Create(ISimulation* simulation);

    protected:
        HIVTransmissionReporter();
        virtual ~HIVTransmissionReporter();

        // BaseTextReport
        virtual std::string GetHeader() const ;

        //StiTransmissinoReporter methods
        virtual void ClearData();
        virtual void CollectOtherData( unsigned int relationshipID,
                                       IIndividualHumanSTI* pPartnerSource,
                                       IIndividualHumanSTI* pPartnerDest );
        virtual std::string GetOtherData( unsigned int relationshipID );

    private:
        std::map<unsigned long, std::string> hiv_report_data ;
    };
}