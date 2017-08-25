/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "SusceptibilityHIV.h"

using namespace Kernel;

class ISusceptibilityHIVFake : public ISusceptibilityHIV
{
public:

    // --------------------------------
    // --- ISusceptibilityHIV Methods
    // --------------------------------
    virtual float GetCD4count() const
    {
        return m_CD4Count ;
    }

    virtual std::vector <float> Generate_forward_CD4( bool ARTYesNo )  override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void  FastForward( const IInfectionHIV * const, float dt ) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void  ApplyARTOnset()                                      override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual ProbabilityNumber GetPrognosisCompletedFraction() const    override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void  TerminateSuppression(float)                          override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    //virtual float GetCD4TimeStep()    const                               override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

    // ---------------------
    // --- ISupport Methods
    // ---------------------
    virtual QueryResult QueryInterface(iid_t iid, void **ppvObject)
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    virtual int32_t AddRef()
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }
    virtual int32_t Release()
    {
        throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented.");
    }

    // ------------------
    // --- Other Methods 
    // ------------------
    void SetCD4Count( float cd4Count )
    {
        m_CD4Count = cd4Count ;
    }

private:
    float m_CD4Count ;
};
