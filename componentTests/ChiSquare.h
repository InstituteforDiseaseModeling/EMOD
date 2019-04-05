/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <vector>
#include "Debug.h"

class ChiSquare
{
public:
    // --------------------------------------------------------------------------------------------
    // --- "Koehler and Larntz suggest that if the total number of observations is at least 10, 
    // --- the number categories is at least 3, and the square of the total number of observations 
    // --- is at least 10 times the number of categories, then the chi-square approximation should
    // --- be reasonable."
    // ---
    // --- "Care should be taken when cell categories are combined (collapsed together) to fix
    // --- problems of small expected cell frequencies. Collapsing can destroy evidence of
    // --- non-independence, so a failure to reject the null hypothesis for the collapsed table 
    // --- does not rule out the possibility of non-independence in the original table."
    // ---
    // --- http://www.basic.northwestern.edu/statguidefiles/gf-dist_ass_viol.html
    // --------------------------------------------------------------------------------------------
    // --- In using the chi-square test of association, you need to observe caution with respect to 
    // --- the expected frequencies. For technical reasons, small expected frequencies cause difficulties 
    // --- in the interpretation of the chi-square coefficient. (Note that what you must be cautious 
    // --- about is the size of the expected frequencies. No special cautions need apply to the observed 
    // --- frequencies. The observed frequencies are the reality, against which you are checking your
    // --- hypothesis that there is no association between the two categorical variables.)
    // --- 
    // --- Different statisticians advocate different minimum acceptable values for the expected 
    // --- frequencies, but almost all would accept a minimum of 10; many would accept a minimum of 5; 
    // --- and some would, in certain circumstances, accept a minimum of 1.
    // ---
    // --- https://www.ischool.utexas.edu/~wyllys/IRLISMaterials/chisquare.pdf
    // --------------------------------------------------------------------------------------------
    // 
    // This method combines the values in the vectors if the expected value/category is less
    // than "minCategorySize".  For example, assume we have the following vectors:
    //    expected = { 100, 50, 20, 10, 6, 2, 2, 0, 1 }
    //    actual   = { 111, 55, 22, 11, 6, 5, 5, 1, 1 }
    // If the "minCategorySize" is 5, then the result should be:
    //    expected = { 100, 50, 20, 10, 6,  5 }
    //    actual   = { 111, 55, 22, 11, 6, 12 }
    // Remember the decision to combine is based on the expected values.
    // See the unit test for more examples.

    static void ReduceDataForChiSquareStatistic( float minCategorySize,
                                                 const std::vector<float>& rExpectedIn,
                                                 const std::vector<float>& rActualIn,
                                                 std::vector<float>& expectedOut,
                                                 std::vector<float>& actualOut )
    {
        expectedOut = rExpectedIn ;
        actualOut = rActualIn ;
        std::vector<float> exp_adj ;
        std::vector<float> act_adj ;

        float exp_next = 0.0 ;
        float act_next = 0.0 ;
        for( int i = 0 ; i < expectedOut.size() ; i++ )
        {
            if( (expectedOut[i] < minCategorySize) && ((i+1) == expectedOut.size()) )
            {
                if( (exp_adj.size() == 0) && ((i+1) == expectedOut.size()) )
                {
                    //done = true ;
                }
                else
                {
                    if( (expectedOut[i] + exp_next) > minCategorySize )
                    {
                        exp_adj.push_back( expectedOut[i] + exp_next ) ;
                        act_adj.push_back( actualOut[i]   + act_next ) ;
                    }
                    else
                    {
                        exp_adj[ exp_adj.size()-1 ] += expectedOut[i] + exp_next ;
                        act_adj[ act_adj.size()-1 ] += actualOut[i]   + act_next ;
                    }
                    exp_next = 0.0 ;
                    act_next = 0.0 ;
                }
            }
            else if( expectedOut[i] < minCategorySize )
            {
                //done = false ;
                exp_next += expectedOut[i] ;
                act_next += actualOut[i] ;
                if( exp_next > minCategorySize )
                {
                    exp_adj.push_back( exp_next );
                    act_adj.push_back( act_next );
                    exp_next = 0.0 ;
                    act_next = 0.0 ;
                }
            }
            else
            {
                if( exp_next > minCategorySize )
                {
                    exp_adj.push_back( exp_next );
                    act_adj.push_back( act_next );
                }
                else
                {
                    exp_adj.push_back( expectedOut[i] + exp_next );
                    act_adj.push_back( actualOut[i]   + act_next );
                }
                exp_next = 0.0 ;
                act_next = 0.0 ;
            }
        }
        if( exp_next > 0.0 )
        {
            exp_adj.push_back( exp_next );
            act_adj.push_back( act_next );
        }
        expectedOut = exp_adj ;
        actualOut   = act_adj ;
    }

    static void CalculateChiSquareStatistic( float minCategorySize,
                                             const std::vector<float>& rExpectedIn,
                                             const std::vector<float>& rActualIn,
                                             float* pStatistic,
                                             int* pDegreesOfFreedom )
    {
        release_assert( rExpectedIn.size() == rActualIn.size() );

        std::vector<float> expected_reduced ;
        std::vector<float> actual_reduced ;

        ReduceDataForChiSquareStatistic( minCategorySize, rExpectedIn, rActualIn, expected_reduced, actual_reduced );

        *pDegreesOfFreedom = int(expected_reduced.size()) - 1 ;
        *pStatistic = 0.0 ;

        for( int i = 0 ; i < expected_reduced.size() ; i++ )
        {
            *pStatistic += (expected_reduced[i] - actual_reduced[i]) * (expected_reduced[i] - actual_reduced[i]) / expected_reduced[i] ;
        }
    }

    static float GetChiSquareCriticalValue( int dof )
    {
        if( dof < 1 )
        {
            return -1.0 ;
        }
        // --------------------------------------------------------------------------------------
        // --- http://www.itl.nist.gov/div898/handbook/eda/section3/eda3674.htm
        // --- Upper-tail critical values of chi-square distribution with ? degrees of freedom 
        // --- for v = 0.95, 0.975
        // --------------------------------------------------------------------------------------
//        float chi_sq_crit_val_095[] =  {  3.841f,  5.991f,  7.815f,  9.488f, 11.070f, 
//                                         12.592f, 14.067f, 15.507f, 16.919f, 18.307f,
//                                         19.675f, 21.026f, 22.362f, 23.685f, 24.996f,
//                                         26.296f, 27.587f, 28.869f, 30.144f, 31.410f } ;

        float chi_sq_crit_val_0975[] = {  5.042f,  7.378f,  9.348f, 11.143f, 12.833f,
                                         14.449f, 16.013f, 17.535f, 19.023f, 20.483f, 
                                         21.920f, 23.337f, 24.736f, 26.119f, 27.488f,
                                         28.845f, 30.191f, 31.526f, 32.852f, 34.170f } ;
        int dof_index = dof -1 ;
        return chi_sq_crit_val_0975[ dof_index ];
    }

};
