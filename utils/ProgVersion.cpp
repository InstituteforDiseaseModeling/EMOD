/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/
#include "stdafx.h"

#include <iostream>
#include <string.h>

#include "ProgVersion.h"

// See the definition of version info
#include "version_info.h"


#define COMBINE_VER(maj,min,rev) \
    (((maj) << 24) | ((min) << 16) | (rev) << 8)

// for unsafe usage of sprintf
#pragma warning(disable: 4996)

ProgDllVersion::ProgDllVersion()
{
    m_nMajor = MAJOR_VERSION;
    m_nMinor = MINOR_VERSION;
    m_nRevision = REVISION_NUMBER; 

    m_nBuild = BUILD_NUMBER;
    strncpy( m_builderName, BUILDER_NAME, VER_LEN );
    strncpy( m_sSccsBranch, SCCS_BRANCH, VER_LEN );
    strncpy( m_sSccsDate, SCCS_DATE, VER_LEN );
    for( int i=0; i<VER_LEN; i++ )
    {
        if( m_sSccsDate[i] == '_' )
            m_sSccsDate[i] = ' ';
    }

    m_nVersion = COMBINE_VER(m_nMajor, m_nMinor, m_nRevision);

    sprintf(m_sVersion, "%d.%d.%d.%d", m_nMajor, m_nMinor, m_nRevision, m_nBuild);
}

const char* ProgDllVersion::getBuildDate()
{
    // on windows this is a (global) static; on linux it's a #define
    return BUILD_DATE;
}

int ProgDllVersion::checkProgVersion(uint8_t nMajor, uint8_t nMinor, uint16_t nRevision)
{
    int ret = 0;
    
    uint32_t num = COMBINE_VER(nMajor, nMinor, nRevision);
    if (num < m_nVersion) 
        ret = -1;
    else if (num > m_nVersion) 
        ret = 1;
     return ret;
}

int ProgDllVersion::checkProgVersion(const char* sVersion)
{
    // -2 for error
    int ret = -2;
    uint8_t maj, min;
    uint16_t rev;
    if (parseProgVersion(sVersion, maj, min, rev))
    {
        ret = checkProgVersion(maj, min, rev);
    }
    return ret;
}

bool ProgDllVersion::parseProgVersion(const char* sVersion, uint8_t& maj, uint8_t& min, uint16_t& rev)
{
    maj = 0;
    min = 0;
    rev = 0;
    
    char sTemp[VER_LEN];
    strncpy(sTemp, sVersion, VER_LEN);
    char* str = strchr(sTemp, '.');
    if (!str) return false;
    
    str[0] = '\0';
    maj = atoi(sTemp);
    
    char* str1 = strchr(str+1, '.');
    if (!str1) return false;
    
    str1[0] = '\0';
    min = atoi(str+1);
    
    char* str2 = strchr(str1+1, '.');
    if (!str2) return false;
    
    str2[0] = '\0';
    rev = atoi(str1+1);
    
    return true;
}

#pragma warning (default: 4996)
