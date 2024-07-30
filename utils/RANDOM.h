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
#include <set>
#include<emmintrin.h> // for __m128i

#include "ISerializable.h"

namespace Kernel
{
    // ------------------------------------------------------------------------
    // --- RANDOMBASE
    // ------------------------------------------------------------------------
    class RANDOMBASE : public ISerializable
    {
    public:

        RANDOMBASE( size_t nCache );
        virtual ~RANDOMBASE();

        bool SmartDraw( float prob );

        uint32_t ul();  // Returns a random 32 bit number.
        float e();      // Returns a randon float between 0 and 1.

        // Finds an uniformally distributed number less than N
        // The 16-bit version requires one less random number than
        // the 32-bit version, i.e. less overhead.
        uint16_t uniformZeroToN16( uint16_t N );
        uint32_t uniformZeroToN32( uint32_t N );

        // randomRound() - rounds the 'val' to the nearest integer but randomly rounds it up or down.
        // If the input value is 5.3, then 70% of the time it should return 5 and 30% of the time 6.
        uint32_t randomRound( float val );

        std::set<uint32_t> chooseMofN( uint32_t M, uint32_t N );

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
        uint64_t binomial_true(uint64_t=1, double=1.0);
        double time_varying_rate_dist( const std::vector <float>& v_rate, float timestep, float rate);

        // Return a vector containing a binomial approximation for each fraction of N such that
        // all of the selections are from the same pool N.  The total sum of the returned
        // vector should be an approximation of the sum of the fractions times N.  That is,
        // Sum(rFractions) * N ~= Sum(returned vector) such that Sum(returned vector) <= N.
        std::vector<uint64_t> multinomial_approx( uint64_t N, const std::vector<float>& rFractions );

        float rand_gamma( float k, float theta );
        uint64_t negative_binomial( float n, float p );
        std::vector<uint32_t> multivariate_hypergeometric( const std::vector<uint32_t>& rColors, uint32_t nsample );

        double get_cdf_random_num_precision();

    protected:

        virtual void fill_bits();
        void bits_to_float();

        int64_t numpy_hypergeometric_sample( int64_t good, int64_t bad, int64_t sample );
        uint64_t numpy_binomial_inversion( uint64_t n, double p );
        uint64_t numpy_binomial_btpe( uint64_t n, double p );

        size_t    cache_count;
        size_t    index;
        uint32_t* random_bits;
        float*    random_floats;
    
        static void serialize( IArchive&, RANDOMBASE* );
        
        bool   bGauss;
        double eGauss_;

        // precision of gamma-distributed random number
        static double cdf_random_num_precision;
    };

    // ------------------------------------------------------------------------
    // --- LINEAR_CONGRUENTIAL
    // ------------------------------------------------------------------------
    // MARSAGLIA, GEORGE. (1972). The Structure of Linear Congruential Sequences.
    // 10.1016/B978-0-12-775950-0.50013-3.

    class LINEAR_CONGRUENTIAL : public RANDOMBASE
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        LINEAR_CONGRUENTIAL( uint32_t iSequence = 0x31415926, size_t nCache = 0 );
       ~LINEAR_CONGRUENTIAL();

    protected:
        virtual void fill_bits() override;

        uint32_t iSeq;

        DECLARE_SERIALIZABLE( LINEAR_CONGRUENTIAL );
    };

    // ------------------------------------------------------------------------
    // --- PSEUDO_DES
    // ------------------------------------------------------------------------
    // Numerical Recipes in C, 2nd ed. Press, William H. et. al, 1992.

    class PSEUDO_DES : public RANDOMBASE
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        PSEUDO_DES( uint64_t iSequence = 0, size_t nCache = 0 );
        ~PSEUDO_DES();

    protected:
        virtual void fill_bits() override;

        uint32_t iSeq;
        uint32_t iNum;

        DECLARE_SERIALIZABLE( PSEUDO_DES );
    };

    // ------------------------------------------------------------------------
    // --- AES_COUNTER
    // ------------------------------------------------------------------------
    // This was created by Chris Lorton based on AES in CTR Mode encryption as implemented in 
    // "Intel (R) Advanced Encryption Standard (AES) New Instruction Set" whitepaper:
    // https://software.intel.com/sites/default/files/article/165683/aes-wp-2012-09-22-v01.pdf

#ifdef WIN32
    #define ALIGN16  __declspec (align(16))
#else
    #define ALIGN16  __attribute__ ((aligned(16)))
#endif

    #define NUM_KEYS_IN_SCHEDULE (15)

    typedef struct KEY_SCHEDULE 
    {
        ALIGN16 __m128i KEY[ NUM_KEYS_IN_SCHEDULE ];
        uint32_t nr;
    } AES_KEY;

    class AES_COUNTER : public RANDOMBASE
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        AES_COUNTER( uint64_t iSequence = 0, size_t nCache = 0 );
        ~AES_COUNTER();

    protected:
        virtual void fill_bits() override;

    private:
        AES_KEY     m_keySchedule;
        uint64_t    m_nonce;
        uint32_t    m_iteration;

        DECLARE_SERIALIZABLE( AES_COUNTER );
    };
}
