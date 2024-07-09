
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