/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include <sstream>
#include "IReport.h"


namespace Kernel 
{
    // BaseTextReport is an abstract base class for reports whose output data is text formatted.
    // The class handles managing a stringstream and writing this to a file.  It is up to the
    // derived classes to format each line.  The class that derives from BaseTextReport must
    // implement GetHeader() so that the text file has one set of text at the beginning of the file.
    class BaseTextReport : public BaseReport
    {
    public:
        BaseTextReport( const std::string& rReportName, bool everyTimeStep = true );
        virtual ~BaseTextReport();

        // ------------
        // --- IReport 
        // ------------
        virtual void Initialize( unsigned int nrmSize );
        virtual void EndTimestep(float currentTime, float dt);
        virtual void Reduce();
        virtual void Finalize();

        virtual void BeginTimestep() {};
        virtual bool IsCollectingIndividualData( float currentTime, float dt ) const { return false; };
        virtual void LogIndividualData( IIndividualHuman* individual ) {};
        virtual void LogNodeData( INodeContext* pNC ) {};

        virtual std::string GetReportName() const { return report_name; }

    protected:
        // Return the text to put at the beginning of the file.
        virtual std::string GetHeader() const = 0 ;

        // Return the full path name of the file to be written
        virtual std::string GetOutputFilePath() const ;

        // Return the handle to the stringstream that is collecting the data.
        std::stringstream& GetOutputStream() { return output_stream; }

        void SetReportName(const std::string& new_name);
        void AddHeaderLine(bool add_endl);

        void GetDataFromOtherCores();
        void WriteData( const std::string& rStringData );

        // make protected so derived classes can set in Configure()
        bool write_every_time_step ;
        bool write_header_newline;

    private:
        std::string report_name;
        std::stringstream output_stream;
        std::stringstream reduced_stream;   // For rank 0 only
        ofstream outfile;
    };

}

