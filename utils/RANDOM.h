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

#include <stdint.h>
#include <vector>
#ifndef WIN32
#include<emmintrin.h> // for __m128i
#endif

#include "IdmApi.h"

#define PRNG_COUNT  (1<<20) // Let's start with ~1 million

class IDMAPI RANDOMBASE
{
public:

    RANDOMBASE(uint32_t iSequence = 0, size_t nCache = PRNG_COUNT);
    virtual ~RANDOMBASE();

    uint32_t ul();  // Returns a random 32 bit number.
    float e();      // Returns a randmon float between 0 and 1.

    // i(N) - Returns a random USHORT less than N.
    uint16_t i(uint16_t N)
    {
        uint32_t ulA = ul();
        uint32_t ll = (ulA & 0xFFFFL) * N;
        ll >>= 16;
        ll += (ulA >> 16) * N;
        return (uint16_t) (ll >> 16);
    }

    // Finds an uniformally distributed number between 0 and N
    uint32_t uniformZeroToN(uint32_t N );

    // randomRound() - rounds the 'val' to the nearest integer but randomly rounds it up or down.
    // If the input value is 5.3, then 70% of the time it should return 5 and 30% of the time 6.
    uint32_t randomRound( float val );

#define FLOAT_EXP   8
#define DOUBLE_EXP 11

    double ee();
    
    double eGauss();    // Returns a normal deviate.

    // Added by Philip Eckhoff, Poisson takes in a rate, and returns the number of events in unit time
    // Or equivalently, takes in rate*time and returns number of events in that time
    // Poisson uses a Gaussian approximation for large lambda, while Poisson_true is the fully accurate Poisson
    // expdist takes in a rate and returns the sample from an exponential distribution
    uint64_t Poisson(double=1.0);
    uint32_t Poisson_true(double=1.0);
    double expdist(double=1.0);
    double Weibull(double lambda=1.0, double kappa=1.0);
    double Weibull2(float lambda=1.0, float inv_kappa=1.0);
    double LogLogistic(double lambda=1.0, double kappa=1.0);
    uint64_t binomial_approx(uint64_t=1, double=1.0);
    uint64_t binomial_approx2(uint64_t=1, double=1.0);
    uint64_t binomial_true(uint64_t=1, double=1.0);
    double time_varying_rate_dist( std::vector <float> v_rate, float timestep, float rate);

    // Return a vector containing a binomial approximation for each fraction of N such that
    // all of the selections are from the same pool N.  The total sum of the returned
    // vector should be an approximation of the sum of the fractions times N.  That is,
    // Sum(rFractions) * N ~= Sum(returned vector) such that Sum(returned vector) <= N.
    std::vector<uint64_t> multinomial_approx( uint64_t N, const std::vector<float>& rFractions );

    // M Behrend
    // gamma-distributed random number
    // shape constant k=2
    double get_pi();
    double rand_gamma(double mean);
    double gamma_cdf(double x, double mean);
    double get_cdf_random_num_precision();

    /* Unused
    void  vShuffleFloats(float *pe,uint16_t N); // Shuffles a list of N floats.
    */

protected:

    virtual void fill_bits();
    void bits_to_float();

    uint32_t iSeq;

    uint32_t* random_bits;
    float*    random_floats;
    size_t    index;
    size_t    cache_count;

    bool   bGauss;
    double eGauss_;

    // precision of gamma-distributed random number
    static double cdf_random_num_precision;
    static double tan_pi_4;
    static double pi;
};

#if 0
template<class Archive>
void serialize(Archive & ar, RANDOMBASE &rng, const unsigned int /* file_version */)
{
    ar & rng.iSeq;
    ar & rng.bSeq;
    ar & rng.bGauss;
    ar & rng.eGauss_;
}
#endif

class IDMAPI RANDOM : public RANDOMBASE
{
public:
    RANDOM(uint32_t iSequence, size_t nCache = PRNG_COUNT) : RANDOMBASE(iSequence, nCache), bSeq(true) {
// clorton        assert(false); // LCG not supported yet
    }
        
    RANDOM() : RANDOMBASE(), bSeq(false)
    {
// clorton        assert(false); // LCG not supported yet
        if (!bSeq)
        {
            iSeq = 0x31415926;
            bSeq = true;
        }
    }

   ~RANDOM() {}

protected:

    virtual void fill_bits() override;

    bool     bSeq;
};

// Numerical Recipes in C, 2nd ed. Press, William H. et. al, 1992.
class IDMAPI PSEUDO_DES : public RANDOMBASE
{
public:
    PSEUDO_DES(uint32_t iSequence = 0, size_t nCache = PRNG_COUNT) : RANDOMBASE(iSequence, nCache), iNum(0)
    {
    }

    ~PSEUDO_DES() { }

protected:

    virtual void fill_bits() override;

    uint32_t iNum;
};

#if 0
template<class Archive>
void serialize(Archive & ar, PSEUDO_DES& rng, const unsigned int /* file_version */)
{
    ar & boost::serialization::base_object<RANDOMBASE>(rng);
    ar & rng.iNum;
}
#endif

#ifdef WIN32
#define ALIGN16  __declspec (align(16))
#else
#define ALIGN16  __attribute__ ((aligned(16)))
#endif

typedef struct KEY_SCHEDULE {
    ALIGN16 __m128i KEY[15];
    unsigned int nr;
} AES_KEY;

class IDMAPI AES_COUNTER : public RANDOMBASE
{
public:
    AES_COUNTER(uint32_t iSequence, uint32_t rank, size_t nCache = PRNG_COUNT);
    ~AES_COUNTER();

protected:
    virtual void fill_bits() override;

private:
    AES_KEY     m_keySchedule;
    uint64_t    m_nonce;
    uint32_t    m_iteration;
};
