
#include "stdafx.h"

#include "TransmissionGroupsUtils.h"
#include "Exceptions.h"
#include "Log.h"

SETUP_LOGGING( "TransmissionGroupsUtils" )

namespace Kernel
{
    void TransmissionGroupsUtils::checkForValidValueListSize( const PropertyValueList_t& values )
    {
        if (values.size() == 0)
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Empty list of property values.");
        }
    }

    void TransmissionGroupsUtils::checkForValidScalingMatrixSize( const ScalingMatrix_t& scalingMatrix, const PropertyValueList_t& values )
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

    void TransmissionGroupsUtils::CheckForValidStrainListSize( const StrainIdentitySet_t& strains )
    {
        // We need at least one strain, even if it's a dummy.
        if (strains.size() == 0)
        {
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Strain list is not allowed to be empty.");
        }
    }

    void TransmissionGroupsUtils::InitializeCumulativeMatrix(ScalingMatrix_t& cumulativeMatrix)
    {
        cumulativeMatrix.clear();

        // [[1.0]]
        MatrixRow_t row(1, 1.0f);           // one element == 1.0f
        cumulativeMatrix.push_back(row);    // one row
    }

    void TransmissionGroupsUtils::AggregatePropertyMatrixWithCumulativeMatrix( const ScalingMatrix_t& propertyMatrix, ScalingMatrix_t& cumulativeMatrix )
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
}
