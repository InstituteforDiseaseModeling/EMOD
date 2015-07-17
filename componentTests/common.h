/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <iostream>

#define HEADER()  { std::cout << std::endl << std::endl << "--===##### " << m_details.suiteName << "::" << m_details.testName << " #####===--" << std::endl << std::endl; }
#define FOOTER()  { std::cout << std::endl << "---------- " << m_details.suiteName << "::" << m_details.testName << " ----------" << std::endl << std::endl; }
