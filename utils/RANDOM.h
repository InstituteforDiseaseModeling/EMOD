/******************************Module*Header*******************************\
* Module Name: random.hxx                                                  *
*                                                                          *
* A random number class.  It depends on a 32 bit ULONG type and 16 bit     *
* USHORT type.                                                             *
*                                                                          *
* Created: 25-Mar-1994 12:34:57                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1994 Charles Whitmer                                       *
\**************************************************************************/

#pragma once

#include "IdmApi.h"
#include "BoostLibWrapper.h"
#include "Serializer.h" // For IJsonSerializable
#include <stdint.h>

// helper macros for verifying 

#define RNG_VALIDATE(call) RNG_VALIDATE_BLOCK(#call, call)

#define RNG_VALIDATE_BLOCK(name, block)\
    VALIDATE(boost::format(">%1%: %2%") % name % randgen->LastUL());\
    block;\
    VALIDATE(boost::format("<%1%: %2%") % name % randgen->LastUL());\

#define RNG_CHECK(call) RNG_CHECK_BLOCK(#call, call)

#define RNG_CHECK_BLOCK(name,block) block;
    //LOG_INFO_F("RNG_CHECK: %d Before     %s\n", randgen->ul(), name);\
    //block;\
    //LOG_INFO_F("RNG_CHECK: %d After      %s\n", randgen->ul(), name);

#define XRNG_CHECK_BLOCK(name, block) block;
#define XRNG_CHECK(block) block;

#define __ULONG     uint32_t
#define __UINT      uint32_t
#define __BOOL      bool
#define __USHORT    uint16_t
#define __ULONGLONG uint64_t

class IDMAPI RANDOMBASE
{
public:

    RANDOMBASE(__ULONG iSequence) :
        iSeq(iSequence),
        bSeq(true),
        last_ul(0),
        bWrite(false),
        bGauss(false)
    {
    }

    RANDOMBASE();
    virtual ~RANDOMBASE();

    __ULONG iSequence() { return iSeq; }

    virtual __ULONG ul() = 0;   // Returns a random 32 bit number.
    __ULONG LastUL()   { return last_ul; }

    // i(N) - Returns a random USHORT less than N.
    __USHORT i(__USHORT N)
    {
        __ULONG ulA = ul();
        __ULONG ll = (ulA & 0xFFFFL) * N;
        ll >>= 16;
        ll += (ulA >> 16) * N;
        return (__USHORT) (ll >> 16);
    }


#define FLOAT_EXP   8
#define DOUBLE_EXP 11

//#define CHECK_RNG_SEQUENCE // debug option to make it easy to visualize the random number sequence

    // e() - Returns a random float between 0 and 1.
    float e()
    {
        union {float e; __ULONG ul;} e_ul;
        
        e_ul.e = 1.0f;
        e_ul.ul += (ul() >> (FLOAT_EXP+1)) | 1;
        float _e =  e_ul.e - 1.0f;
#ifdef CHECK_RNG_SEQUENCE
        LOG_INFO_F("e() = %f\n", _e);
#endif
        return _e;
    }

    double ee();
    
    double eGauss();    // Returns a normal deviate.

    // Added by Philip Eckhoff, Poisson takes in a rate, and returns the number of events in unit time
    // Or equivalently, takes in rate*time and returns number of events in that time
    // Poisson uses a Gaussian approximation for large lambda, while Poisson_true is the fully accurate Poisson
    // expdist takes in a rate and returns the sample from an exponential distribution
    unsigned long long int Poisson(double=1.0);
    unsigned long int Poisson_true(double=1.0);
    double expdist(double=1.0);
    double Weibull(double lambda=1.0, double kappa=1.0);
    double Weibull2(float lambda=1.0, float inv_kappa=1.0);
    double LogLogistic(double lambda=1.0, double kappa=1.0);
    uint64_t binomial_approx(uint64_t=1, double=1.0);
    uint64_t binomial_approx2(uint64_t=1, double=1.0);
    uint64_t binomial_true(uint64_t=1, double=1.0);
    double time_varying_rate_dist( std::vector <float> v_rate, float timestep, float rate);

    
    // M Behrend
    // gamma-distributed random number
    // shape constant k=2
    double get_pi();
    double rand_gamma(double mean);
    double gamma_cdf(double x, double mean);
    double get_cdf_random_num_precision();

    /* Unused
    void  vShuffleFloats(float *pe,__USHORT N); // Shuffles a list of N floats.
    */

protected:
    __ULONG iSeq;
    __BOOL  bSeq;
    __ULONG last_ul;

private:
    __BOOL  bWrite;
    __BOOL  bGauss;
    double  eGauss_;
    static int cReg;

    // precision of gamma-distributed random number
    static double cdf_random_num_precision;
    static double tan_pi_4;
    static double pi;

#if USE_BOOST_SERIALIZATION
    friend class boost::serialization::access;
    template<class Archive>
    friend void serialize(Archive & ar, RANDOMBASE &rng, const unsigned int /* file_version */);
#endif

#if USE_JSON_SERIALIZATION
     public:

         // IJsonSerializable Interfaces
         virtual void JSerialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper ) const;
         virtual void JDeserialize( Kernel::IJsonObjectAdapter* root, Kernel::JSerializer* helper );
#endif
};

#if USE_BOOST_SERIALIZATION
template<class Archive>
void serialize(Archive & ar, RANDOMBASE &rng, const unsigned int /* file_version */)
{
    ar & rng.iSeq;
    ar & rng.bSeq;
    ar & rng.bWrite;
    ar & rng.bGauss;
    ar & rng.eGauss_;
}
#endif

class IDMAPI RANDOM : public RANDOMBASE
{
public:
    RANDOM(__ULONG iSequence) : RANDOMBASE(iSequence) {}
        
    RANDOM() : RANDOMBASE()
    {
        assert(false); // LCG not supported yet
        if (!bSeq)
        {
            iSeq = 0x31415926;
            bSeq = true;
        }
    }

   ~RANDOM() {}
    
    __ULONG ul();
};

// Numerical Recipes in C, 2nd ed. Press, William H. et. al, 1992.
class IDMAPI PSEUDO_DES : public RANDOMBASE
{
    __ULONG   iNum;

public:
    PSEUDO_DES(__ULONG iSequence) : RANDOMBASE(iSequence)
    {
        iNum = 0;
    }
        
    PSEUDO_DES() : RANDOMBASE()
    {
        if (!bSeq)
        {
            iSeq = 0;
            bSeq = true;
        }
        iNum = 0;
    }

    __ULONG get_iNum() { return iNum; }

   ~PSEUDO_DES()
    {
        iSeq++;
    }
    
    __ULONG ul();

private:
#if USE_BOOST_SERIALIZATION
    friend class boost::serialization::access;
    template<class Archive>
    friend void serialize(Archive & ar, PSEUDO_DES& rng, const unsigned int /* file_version */);
#endif
};

#if USE_BOOST_SERIALIZATION
template<class Archive>
void serialize(Archive & ar, PSEUDO_DES& rng, const unsigned int /* file_version */)
{
    ar & boost::serialization::base_object<RANDOMBASE>(rng);
    ar & rng.iNum;
}
#endif

union __ULONGLONG_
{
    struct 
    {
        __ULONG Low;
        __ULONG High;
    };
    __ULONGLONG Quad;
};

void IntializeRNG( int RANDOM_TYPE, int Run_Number, int rank );
