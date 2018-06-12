/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

// Product version numbers.
// Definition for version number in the forma of major.minor.revision.build, 1.2.1432.0
// Major version: Major feature release, e.g., Version 1.0 release with major features of Malaria, Polio modeling,
// Minor version: Minor feature release, e.g., Version 1.2 release with particular vector species for Malaria modeling,
//                                             which could be our iteration number
// Revision number: Bug fixing release, e.g.,  Version 1.2.3 release with model serialization bug fixing
//                                             which could be automatically from svn revision number,
// Build number:    Number of builds done (none zero values) for this revision with local modification (monotonically increasing)  
// Note: 1. All numbers could be jumped to indicate significant changes
//       2. The build script will first set these four numbers before start the building process 

#define VER_LEN 256

#ifdef __GNUC__
#include <stdint.h>
#endif

class ProgDllVersion
{
public:
    ProgDllVersion();

    virtual ~ProgDllVersion () { };

    uint8_t getMajorVersion() { return m_nMajor; }
    uint8_t getMinorVersion() { return m_nMinor; }
    uint16_t getRevisionNumber() { return m_nRevision; }
    
    uint32_t getBuildNumber() { return m_nBuild; }
    const char* getSccsBranch() { return m_sSccsBranch; }
    const char* getSccsDate() { return m_sSccsDate; }
    const char* getBuilderName() { return m_builderName; }
    const char* getBuildDate(); // { return BUILD_DATE; }

    const char* getVersion() { return m_sVersion; }

    // Check the input version (0.0.0 with build number excluded) against this program's version
    // return 0: exact the same version
    //        1: input version is higher than this program's version
    //       -1: input version is lower than this program's version
    // 
    int checkProgVersion(uint8_t nMajor, uint8_t nMinor, uint16_t nRevision);
    int checkProgVersion(const char* sVersion);
    
protected:
    bool parseProgVersion(const char* sVersion, uint8_t& maj, uint8_t& min, uint16_t& rev);

private:
    uint8_t m_nMajor; 
    uint8_t m_nMinor;
    uint16_t m_nRevision;

    uint32_t m_nBuild;
    char m_builderName[VER_LEN];
    char m_sSccsBranch[VER_LEN];
    char m_sSccsDate[VER_LEN];

    uint32_t m_nVersion;
    char m_sVersion[VER_LEN];
};
