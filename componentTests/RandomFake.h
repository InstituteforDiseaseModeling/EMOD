
#pragma once

#include "RANDOM.h"

class RandomFake : public Kernel::RANDOMBASE
{
public:
    RandomFake()
        : RANDOMBASE(8)
        , m_SelectBits()
    {
    }

    void SetUL( uint32_t ul )
    {
        m_SelectBits.clear();
        for( size_t i = 0; i < cache_count; ++i )
        {
            m_SelectBits.push_back( ul );
        }
        fill_bits();
        bits_to_float();
        index = 0;
    }

    virtual Kernel::QueryResult QueryInterface( Kernel::iid_t iid, void** pinstance) { return Kernel::QueryResult::e_NOINTERFACE ; };
    virtual int32_t AddRef()  { return -1 ; };
    virtual int32_t Release() { return -1 ; };

    void SetUL( const std::vector<uint32_t>& rBits )
    {
        m_SelectBits.clear();
        for( auto bit_val : rBits )
        {
            m_SelectBits.push_back( bit_val );
        }
        fill_bits();
        bits_to_float();
        index = 0;
        bGauss = false;
    }

protected:
    virtual void fill_bits() override
    { 
        memset( random_bits, 0, sizeof(uint32_t)*cache_count );
        for( size_t i = 0; i < m_SelectBits.size(); ++i)
        {
            random_bits[i] = m_SelectBits[i];
        }
    }

private:
    std::vector<uint32_t> m_SelectBits ;
};
