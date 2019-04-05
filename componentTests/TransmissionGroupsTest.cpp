/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "UnitTest++.h"
#include "../utils/Common.h"
#include "SimulationConfig.h"
#include "IdmMpi.h"
#include "RANDOM.h"
#include "TransmissionGroupsFactory.h"
#include "StrainIdentity.h"
#include "TransmissionGroupMembership.h"
#include "Infection.h"

using namespace std;
using namespace Kernel;

#define CONTACT_DECAY       (1.0f)
#define ENVIRONMENTAL_DECAY (0.5f)

SUITE(TransmissionGroupsTest)
{
    struct TransmissionGroupsTestFixture
    {
        TransmissionGroupsTestFixture()
            : risk("RISK")
            , high("HIGH")
            , medium("MEDIUM")
            , low("LOW")
            , risk_values{ high, medium, low }
            , location("LOCATION")
            , bothell("BOTHELL")
            , bellevue("BELLEVUE")
            , redmond("REDMOND")
            , renton("RENTON")
            , location_values{ bothell, bellevue, redmond, renton }
            , risk_matrix{
                MatrixRow_t{ 1, 0, 0 },
                MatrixRow_t{ 0, 1, 0 },
                MatrixRow_t{ 0, 0, 1 } }
            , location_matrix{
                MatrixRow_t{ 0.5f,   0.25f,  0.25f,  0.0f },
                MatrixRow_t{ 0.125f, 0.625f, 0.125f, 0.125f },
                MatrixRow_t{ 0.0f,   0.5f,   0.5f,   0.0f },
                MatrixRow_t{ 0.0f,   0.125f, 0.125f, 0.75f } }
            , p_simulation_config( new SimulationConfig() )
            , number_basestrains_save( InfectionConfig::number_basestrains )
            , number_substrains_save( InfectionConfig::number_substrains )
        {
            /* Set up environment: */
            JsonConfigurable::ClearMissingParameters();
            Environment::Finalize();
            Environment::setLogger(new SimpleLogger(Logger::tLevel::WARNING));
            Environment::setSimulationConfig( p_simulation_config );

            IPFactory::DeleteFactory();
            IPFactory::CreateFactory();

            std::map<std::string, float> risk_ip_values{ {high, 0.5f}, {medium, 0.25f}, {low, 0.25f} };
            IPFactory::GetInstance()->AddIP(1, risk, risk_ip_values);

            std::map<std::string, float> location_ip_values{ {bothell, 0.25f}, {bellevue, 0.25f }, {redmond, 0.25f}, {renton, 0.25f} };
            IPFactory::GetInstance()->AddIP(1, location, location_ip_values);

            InfectionConfig::number_basestrains = 2;
            InfectionConfig::number_substrains  = 2;
        }

        ~TransmissionGroupsTestFixture()
        {
            InfectionConfig::number_basestrains = number_basestrains_save;
            InfectionConfig::number_substrains  = number_substrains_save;

            IPFactory::DeleteFactory();
            Environment::Finalize();
            delete p_simulation_config;
        }

        std::string risk, high, medium, low;
        PropertyValueList_t risk_values;

        std::string location, bothell, bellevue, redmond, renton;
        PropertyValueList_t location_values;

        ScalingMatrix_t risk_matrix;
        ScalingMatrix_t location_matrix;

        SimulationConfig* p_simulation_config;
        PSEUDO_DES rng;

        uint32_t number_basestrains_save;
        uint32_t number_substrains_save;
    };

    TEST_FIXTURE(TransmissionGroupsTestFixture, TestStrainAwareTxGroupsSingleProperty)
    {
        // Instantiate StrainAwareTransmissionGroups with single property
        unique_ptr<ITransmissionGroups> txGroups( TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::StrainAwareGroups, &rng) );
        txGroups->AddProperty(risk, risk_values, risk_matrix);
        txGroups->Build(CONTACT_DECAY, 2, 2);

        // Deposit contagion
        StrainIdentity strain00 = StrainIdentity(0, 0);
        StrainIdentity strain01 = StrainIdentity(0, 1);
        StrainIdentity strain10 = StrainIdentity(1, 0);
        StrainIdentity strain11 = StrainIdentity(1, 1);
        TransmissionGroupMembership_t membership;

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, high } }, membership);
        txGroups->UpdatePopulationSize(membership, 2, 1.0f);
        txGroups->DepositContagion(strain00, 0.25, membership);
        txGroups->DepositContagion(strain01, 0.25, membership);
        txGroups->DepositContagion(strain10, 0.25, membership);
        txGroups->DepositContagion(strain11, 0.25, membership);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, medium } }, membership);
        txGroups->UpdatePopulationSize(membership, 1, 2.0f);
        txGroups->DepositContagion(strain00, 0.125, membership);
        txGroups->DepositContagion(strain01, 0.125, membership);
        txGroups->DepositContagion(strain10, 0.125, membership);
        txGroups->DepositContagion(strain11, 0.125, membership);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, low } }, membership);
        txGroups->UpdatePopulationSize(membership, 4, 1.0f);
        txGroups->DepositContagion(strain00, 0.0625, membership);
        txGroups->DepositContagion(strain01, 0.0625, membership);
        txGroups->DepositContagion(strain10, 0.0625, membership);
        txGroups->DepositContagion(strain11, 0.0625, membership);

        txGroups->EndUpdate();

        float contagion = txGroups->GetTotalContagion();
        CHECK_EQUAL( ((0.25f * 4) + (0.125f * 4) + (0.0625f) * 4) / (2 + 2 + 4), contagion );

        contagion = txGroups->GetContagionByProperty(IPKeyValue(risk, high));
        CHECK_EQUAL( (0.25f * 4)/8, contagion);

        contagion = txGroups->GetContagionByProperty(IPKeyValue(risk, medium));
        CHECK_EQUAL( (0.125f * 4)/8, contagion);

        contagion = txGroups->GetContagionByProperty(IPKeyValue(risk, low));
        CHECK_EQUAL( (0.0625f * 4)/8, contagion);

        // Groups and properties have a 1:1 relationship in this setup.
        // Don't bother checking GetTotalContagionForGroup().
    }

    TEST_FIXTURE(TransmissionGroupsTestFixture, TestStrainAwareTxGroupsMultipleProperties)
    {
        // Instantiate StrainAwareTransmissionGroups with two properties
        unique_ptr<ITransmissionGroups> txGroups( TransmissionGroupsFactory::CreateNodeGroups(TransmissionGroupType::StrainAwareGroups, &rng) );
        txGroups->AddProperty(risk, risk_values, risk_matrix);
        txGroups->AddProperty(location, location_values, location_matrix);
        txGroups->Build(CONTACT_DECAY, 2, 1);

        // Deposit contagion
        StrainIdentity strain0 = StrainIdentity(0, 0);
        StrainIdentity strain1 = StrainIdentity(1, 0);
        TransmissionGroupMembership_t membership;

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, medium },{ location, bothell } }, membership);
        txGroups->UpdatePopulationSize(membership, 8, 1.0f);
        txGroups->DepositContagion(strain0, 0.25f, membership);
        txGroups->DepositContagion(strain1, 0.75f, membership);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, low },{ location, bellevue } }, membership);
        txGroups->UpdatePopulationSize(membership, 1, 4.0f);
        txGroups->DepositContagion(strain0, 2.25f, membership);
        txGroups->DepositContagion(strain1, 0.75f, membership);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, low },{ location, redmond } }, membership);
        txGroups->UpdatePopulationSize(membership, 2, 1.0f);
        txGroups->DepositContagion(strain0, 1.25f, membership);
        txGroups->DepositContagion(strain1, 3.75f, membership);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, high },{ location, renton } }, membership);
        txGroups->UpdatePopulationSize(membership, 1, 2.0f);
        txGroups->DepositContagion(strain0, 5.25f, membership);
        txGroups->DepositContagion(strain1, 1.75f, membership);

        txGroups->EndUpdate();

        float contagion = txGroups->GetContagionByProperty(IPKeyValue(risk, high));
        CHECK_EQUAL(7.0f/16, contagion);

        contagion = txGroups->GetContagionByProperty(IPKeyValue(risk, medium));
        CHECK_EQUAL(1.0f/16, contagion);

        contagion = txGroups->GetContagionByProperty(IPKeyValue(risk, low));
        CHECK_EQUAL(8.0f/16, contagion);

        contagion = txGroups->GetContagionByProperty(IPKeyValue(location, bothell));
        CHECK_EQUAL(0.875f/16, contagion);

        contagion = txGroups->GetContagionByProperty(IPKeyValue(location, bellevue));
        CHECK_EQUAL(5.5f/16, contagion);

        contagion = txGroups->GetContagionByProperty(IPKeyValue(location, redmond));
        CHECK_EQUAL(4.0f/16, contagion);

        contagion = txGroups->GetContagionByProperty(IPKeyValue(location, renton));
        CHECK_EQUAL(5.625f/16, contagion);

        // Check contagion by group
        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, high },{ location, bothell } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.000f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, medium },{ location, bothell } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.500f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, low },{ location, bothell } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.375f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, high },{ location, bellevue } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.875f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, medium },{ location, bellevue } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.250f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, low },{ location, bellevue } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(4.375f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, high },{ location, redmond } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.875f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, medium },{ location, redmond } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.250f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, low },{ location, redmond } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(2.875f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, high },{ location, renton } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(5.250f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, medium },{ location, renton } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.000f/16, contagion);

        txGroups->GetGroupMembershipForProperties(tProperties{ { risk, low },{ location, renton } }, membership);
        contagion = txGroups->GetTotalContagionForGroup(membership);
        CHECK_EQUAL(0.375f/16, contagion);
    }

    TEST_FIXTURE(TransmissionGroupsTestFixture, TestStrainAwareTxGroupsBadParams)
    {
        // Instantiate StrainAwareTransmissionGroups with single property
        unique_ptr<ITransmissionGroups> txGroups( TransmissionGroupsFactory::CreateNodeGroups( TransmissionGroupType::StrainAwareGroups, &rng ) );
        txGroups->AddProperty(risk, risk_values, risk_matrix);
        txGroups->Build(CONTACT_DECAY, 2, 2);
        txGroups->EndUpdate();

        // location is not a property used in this transmission setting.
        CHECK_EQUAL( 0.0f, txGroups->GetContagionByProperty( IPKeyValue(location, bothell) ) );
    }
}

/* hint-matrices.xlsx encoded below with the following: base64.b64encode(zlib.compress(open('hint-matrices.xlsx', 'rb').read()))
** Use this code to recreate the Excel file:
#!/usr/bin/python

import base64
import zlib

open('hint-matrices.xlsx','wb').write(zlib.decompress(base64.b64decode(b'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')))
*/
