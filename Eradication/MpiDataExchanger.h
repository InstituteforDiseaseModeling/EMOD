/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <functional>
#include <vector>


namespace Kernel
{
    struct IdmDateTime;
    struct IArchive;
    struct ISerializable;

    // This function is used to perform an action on data for the current process.
    // For example, if an individual is migrating to a node in this process, then
    // this function would handle the code for managing that.
    typedef std::function<void(int myRank)> WithSelfFunc;

    // This function is used to write data to the IArchive object that should be sent
    // to the other processes.  The function should use the "toRank" argument to determine
    // what data should be sent.
    typedef std::function<void(IArchive* writer, int toRank)> SendToOthersFunc;

    // This function is used to read the data from the IArchive object that was sent
    // from the SendToOthersFunc and then dispense it to the appropriate objects.
    typedef std::function<void(IArchive* reader, int fromRank)> ReceiveFromOthersFunc;

    // This function allows the user to clear any buffers used between between each send and receive.
    typedef std::function<void(int rank)> ClearDataFunc;


    // MpiDataExchanger is used to exchange IArchive data with the other cores of the simulation.
    // The user of this class provides the functions that gather the data to send and to dispense
    // the data when it is received.
    class MpiDataExchanger
    {
    public:
        MpiDataExchanger( const char* pName,
                          WithSelfFunc withSelfFunc,
                          SendToOthersFunc toOthersFunc,
                          ReceiveFromOthersFunc fromOthersFunc,
                          ClearDataFunc clearFunc );
        ~MpiDataExchanger();

        void ExchangeData( IdmDateTime& currentTime );

    private:
        void SaveData( uint32_t time_step, 
                       uint32_t source, 
                       uint32_t dest, 
                       char* suffix, 
                       const char* buffer, 
                       size_t size );

        const char*           m_pName;
        WithSelfFunc          m_WithSelfFunc;
        SendToOthersFunc      m_ToOthersFunc;
        ReceiveFromOthersFunc m_FromOthersFunc;
        ClearDataFunc         m_ClearDataFunc;
    };
}