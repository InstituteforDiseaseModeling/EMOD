
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

    virtual void  ApplyARTOnset() override 
    {
    }

    virtual void  FastForward( const IInfectionHIV * const, float dt ) override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual ProbabilityNumber GetPrognosisCompletedFraction() const    override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual void  TerminateSuppression(float)                          override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual bool IsSymptomatic() const                                 override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    //virtual float GetCD4TimeStep()    const                               override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }
    virtual float GetDaysBetweenSymptomaticAndDeath() const            override { throw Kernel::NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "The method or operation is not implemented."); }

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
