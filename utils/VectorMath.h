
#pragma once

#include <vector>

// VectorMath is a collection of methods that perform Linear Algebra type functions
// on an "array" of values.  It uses templates so that one can have different types
// of values - double, float, GeneticProbability.  The methods were originally part
// of just TransmissionGroups.
namespace VectorMath
{
    template<typename T, typename U>
    T dotProduct( const std::vector<T>& vectorOne, const std::vector<U>& vectorTwo )
    {
        size_t vectorSize = min(vectorOne.size(), vectorTwo.size());
        T result = 0.0f;
        for (size_t iElement = 0; iElement < vectorSize; ++iElement)
        {
            result += vectorOne[ iElement ] * vectorTwo[ iElement ];
        }

        return result;
    }

    template<typename T>
    void elementAdd( std::vector<T>& a, const std::vector<T>& b )
    {
        size_t count = min(a.size(), b.size());
        for (size_t index = 0; index < count; ++index)
        {
            a[ index ] += b[ index ];
        }
    }

    template<typename T>
    void elementDivide( std::vector<T>& numerator, const std::vector<T>& denominator )
    {
        size_t count = min(numerator.size(), denominator.size());
        for (size_t index = 0; index < count; ++index)
        {
            if ( denominator[ index ] != 0.0f)
            {
                numerator[ index ] /= denominator[ index ];
            }
            else
            {
                numerator[ index ] = 0.0f;
            }
        }
    }

    template<typename T>
    void scalarAdd( std::vector<T>& vector, float scalar )
    {
        for( T& entry : vector )
        {
            entry += scalar;
        }
    }

    template<typename T>
    void scalarMultiply( std::vector<T>& vector, float scalar )
    {
       for( T& entry : vector )
       {
           entry *= scalar;
       }
    }
}