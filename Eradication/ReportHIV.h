/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <map>
#include "ReportSTI.h"
#include "IndividualEventContext.h"

namespace Kernel {

    class ReportHIV : public ReportSTI, 
                      public IIndividualEventObserver
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportHIV)
    public:
        ReportHIV();
        virtual ~ReportHIV();

        static IReport* ReportHIV::CreateReport() { return new ReportHIV(); }

        virtual bool Configure( const Configuration* inputJson );
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList ) ;

        virtual void LogIndividualData( IndividualHuman* individual );
        virtual void EndTimestep( float currentTime, float dt );
        virtual void Reduce();

        // for IIndividualEventObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *context, 
                                    const std::string& StateChange );

#if USE_JSON_SERIALIZATION
        // For JSON serialization
        virtual void JSerialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) const {};
        virtual void JDeserialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) {};
#endif
        // ISupports
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject) { return Kernel::e_NOINTERFACE; }
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    protected:
        virtual void populateSummaryDataUnitsMap( std::map<std::string, std::string> &units_map );

        float num_acute;
        float num_latent;
        float num_aids;
        float num_hiv_cd4_lo_non_ART;
        float num_hiv_cd4_hi_non_ART;
        float num_hiv_cd4_lo_on_ART;
        float num_hiv_cd4_hi_on_ART;
        float num_on_ART;
        float num_ART_dropouts;
        unsigned int num_events ;
        std::map< std::string, unsigned int> event_to_counter_map ;
        std::vector< std::string > eventTriggerList ;
        std::vector<INodeTriggeredInterventionConsumer*> ntic_list ;
    };

}

