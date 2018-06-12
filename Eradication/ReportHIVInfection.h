/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <sstream>
#include "BaseTextReport.h"

namespace Kernel {
    struct ISimulation;

    class ReportHIVInfection : public BaseTextReport
    {
        GET_SCHEMA_STATIC_WRAPPER(ReportHIVInfection)
    public:
        ReportHIVInfection( const ISimulation * sim = nullptr );
        static IReport* ReportHIVInfection::Create(const ISimulation * parent) { return new ReportHIVInfection( parent ); }

        virtual bool Configure( const Configuration* inputJson );
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const ;
        virtual void LogIndividualData( IIndividualHuman* individual );

    protected:
        virtual std::string GetHeader() const ;

    private:
        const ISimulation * _parent;
        float startYear ; // Year to start collecting data
        float stopYear ;  // Year to stop  collecting data
    };

}

