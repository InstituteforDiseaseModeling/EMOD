/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <string>
#include <functional>
#include "IdmApi.h"
#include "Configure.h"
#include "INodeContext.h"

namespace Kernel
{
    struct INodeContext;
    struct INodeEventContext;
    struct IIndividualHuman;

    class IDMAPI IReport : public JsonConfigurable
    {
    public:
        virtual ~IReport() {} ;

        // ----------------------------
        // --- JsonConfigurable Methods 
        // ----------------------------
        // make public from JsonConfigurable
        virtual bool Configure( const Configuration* inputJson ) = 0;

        // ------------------
        // --- Public Methods
        // ------------------
        virtual void Initialize(unsigned int nrmSize) = 0;
        virtual void CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& demographicNodeIds) = 0;

        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList ) = 0;
        virtual void BeginTimestep() = 0;
        virtual void LogIndividualData( IIndividualHuman* individual ) = 0;
        virtual void LogNodeData( INodeContext* pNC ) = 0;
        virtual void EndTimestep( float currentTime, float dt) = 0;

        // Return true if LogIndividualData() should be called.
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const = 0 ;

        virtual void Reduce() = 0;
        virtual void Finalize() = 0;

        virtual std::string GetReportName() const = 0;
    };

    // This abstract class provides a default implementation to some of pure virtual methods
    // of IReport.  This allows other classes to be IReport's
    // without needing the equivalent of the code below.
    class IDMAPI BaseReport : public IReport
    {
    public:
        virtual ~BaseReport(){};

        // ---------------------
        // --- ISupport Methods
        // ---------------------
        virtual bool Configure( const Configuration* inputJson ) { return true ; };
        virtual Kernel::QueryResult QueryInterface(Kernel::iid_t iid, void **ppvObject) { return Kernel::e_NOINTERFACE; }
        virtual int32_t AddRef()  { return -1 ; }
        virtual int32_t Release() { return -1 ; }

        // -------------------
        // --- IReport Methods
        // -------------------
        virtual void CheckForValidNodeIDs(const std::vector<ExternalNodeId_t>& demographicNodeIds) {};
        virtual void UpdateEventRegistration( float currentTime, 
                                              float dt, 
                                              std::vector<INodeEventContext*>& rNodeEventContextList ) {} ;
    };

    // This is a pointer to a function that is used to create/instantiate reports
    typedef std::function< IReport* (void)> report_instantiator_function_t;
}
