/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "TransmissionGroupsBase.h"
#include "Exceptions.h"

namespace Kernel
{
    TransmissionGroupsBase::TransmissionGroupsBase()
        : propertyNameToMatrixMap()
        , propertyValueToIndexMap()
    {

    }

    void TransmissionGroupsBase::CheckForDuplicatePropertyName( const string& property ) const
    {
        if (propertyNameToMatrixMap.find(property) != propertyNameToMatrixMap.end())
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Duplicated property name.");
        }
    }

    void TransmissionGroupsBase::CheckForValidValueListSize( const PropertyValueList_t& values ) const
    {
        if (values.size() == 0)
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Empty list of property values.");
        }
    }

    void TransmissionGroupsBase::CheckForValidScalingMatrixSize( const ScalingMatrix_t& scalingMatrix, const PropertyValueList_t& values ) const
    {
        int matrixSize = scalingMatrix.size();
        int valueCount = values.size();

        // scalingMatrix must be values.size() x values.size()

        if (matrixSize != valueCount)
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Scaling matrix size (rows) doesn't match property value count.");
        }

        for (int iRow = 0; iRow < scalingMatrix.size(); iRow++)
        {
            if (scalingMatrix[iRow].size() != valueCount)
            {
                throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Scaling matrix size (columns) doesn't match property value count.");
            }
        }
    }

    void TransmissionGroupsBase::AddScalingMatrixToPropertyToMatrixMap( const string& property, const ScalingMatrix_t& scalingMatrix )
    {
        propertyNameToMatrixMap[property] = scalingMatrix;
    }

    void TransmissionGroupsBase::CheckForValidStrainListSize( const StrainIdentitySet_t& strains ) const
    {
        // We need at least one strain, even if it's a dummy.
        if (strains.size() == 0)
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Strain list is not allowed to be empty.");
        }
    }

    void TransmissionGroupsBase::InitializeCumulativeMatrix(ScalingMatrix_t& cumulativeMatrix)
    {
        cumulativeMatrix.clear();

        // [[1.0]]
        MatrixRow_t row(1, 1.0f);           // one element == 1.0f
        cumulativeMatrix.push_back(row);    // one row
    }

    void TransmissionGroupsBase::AddPropertyValuesToValueToIndexMap( const string& propertyName, const PropertyValueList_t& valueSet, int currentMatrixSize )
    {
        ValueToIndexMap_t valueToIndexMap;
        int valueIndex = 0;
        for (auto& value : valueSet)
        {
            valueToIndexMap[value] = valueIndex * currentMatrixSize;
            valueIndex++;
        }

        propertyValueToIndexMap[propertyName] = valueToIndexMap;
    }

    void TransmissionGroupsBase::AggregatePropertyMatrixWithCumulativeMatrix( const ScalingMatrix_t& propertyMatrix, ScalingMatrix_t& cumulativeMatrix )
    {
        int propertyMatrixSize = propertyMatrix.size();
        int currentMatrixSize  = cumulativeMatrix.size();
        MatrixRow_t emptyRow(propertyMatrixSize * currentMatrixSize, 0.0f);
        ScalingMatrix_t aggregateMatrix(propertyMatrixSize * currentMatrixSize, emptyRow);

        for (int propSource = 0; propSource < propertyMatrix.size(); propSource++)
        {
            for (int yB = 0; yB < cumulativeMatrix.size(); yB++)
            {
                for (int propSink = 0; propSink < propertyMatrix[propSource].size(); propSink++)
                {
                    for (int xB = 0; xB < cumulativeMatrix[yB].size(); xB++)
                    {
                        int xResult = (propSource * cumulativeMatrix.size()) + xB;
                        int yResult = (propSink   * cumulativeMatrix.size()) + yB;
                        aggregateMatrix[yResult][xResult] = propertyMatrix[propSource][propSink] * cumulativeMatrix[yB][xB];
                    }
                }
            }
        }

        cumulativeMatrix = aggregateMatrix;
    }

    BEGIN_QUERY_INTERFACE_BODY(TransmissionGroupsBase::ContagionPopulationImpl)
    END_QUERY_INTERFACE_BODY(TransmissionGroupsBase::ContagionPopulationImpl)

    AntigenId TransmissionGroupsBase::ContagionPopulationImpl::GetAntigenId( void ) const
    {
        return (AntigenId)0;
    }

    float TransmissionGroupsBase::ContagionPopulationImpl::GetTotalContagion( void ) const
    {
        return contagionQuantity;
    }

    void TransmissionGroupsBase::ContagionPopulationImpl::ResolveInfectingStrain( StrainIdentity* strainId ) const
    {
        strainId->SetGeneticID(0);
    }

    act_prob_vec_t TransmissionGroupsBase::DiscreteGetTotalContagion(const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
       throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, 
            "The use of DiscreteGetTotalContagion is not supported in \"GENERIC_SIM\".  \
             To use discrete transmission, please use a simulation type derived from either \
             \"STI_SIM\" or \"HIV_SIM\"." ); 
    }
}
