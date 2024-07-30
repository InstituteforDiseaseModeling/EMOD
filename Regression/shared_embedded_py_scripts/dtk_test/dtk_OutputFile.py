#!/usr/bin/python

import os
import json
import pandas as pd
from enum import Enum, auto

class OutputFile:
    """
    A base class carrying the lowest level output file interfaces called by JsonOutput and CsvOutput
    """
    def __init__(self, file):
        self.file = file
        self.filepath = self.filename = None
        if self.isfile():
            self.filepath, self.filename = os.path.split(os.path.abspath(file))

    def isfile(self):
        if os.path.isfile(self.file) and os.access(self.file, os.R_OK):
            return True
        else:
            raise ValueError(f"{self.file} doesn't exist or not readable.")


class TextFile(OutputFile):
    def __init__(self, file, filter_string_list=None):
        super().__init__(file)
        self.lines = []
        self.filtered_lines = []
        if filter_string_list:
            self.filter_lines(filter_string_list)
        else:
            self.read_lines()

    def read_lines(self):
        with open(self.file, 'r') as infile:
            for line in infile:
                self.lines.append(line)

    def filter_lines(self, filter_string_list):
        with open(self.file, 'r') as infile:
            for line in infile:
                for filter_string in filter_string_list:
                    if filter_string in line:
                        self.filtered_lines.append(line)
                        continue


class JsonOutput(OutputFile):
    def __init__(self, file):
        super().__init__(file)
        self.json = self.load_json()

    def load_json(self):
        with open(self.file, 'r') as infile:
            try:
                return json.load(infile)
            except Exception as ex:
                raise ValueError(f"{self.file} is not a loadable json file. Got '{ex}'' when trying to load it.")


class CsvOutput(OutputFile):
    def __init__(self, file):
        super().__init__(file)
        self.df = self.load_csv()

    def load_csv(self):
        with open(self.file, 'r') as infile:
            try:
                return pd.read_csv(infile)
            except Exception as ex:
                raise ValueError(f"Can't read {self.file} into data frame. Got '{ex}' when trying to read it.")


class ReportEventRecorder(CsvOutput):
    class Column(Enum):
        Time = auto()
        Year = auto()
        Node_ID = auto()
        Event_Name = auto()
        Individual_ID = auto()
        Age = auto()
        Age_Year = auto()
        Gender = auto()
        Infected = auto()
        Infectiousness = auto()
        CD4 = auto()

    class Event(Enum):
        NoTrigger = auto()
        Births = auto()
        EveryUpdate = auto()
        EveryTimeStep = auto()
        NewInfectionEvent = auto()
        InfectionCleared = auto()
        TBActivation = auto()
        NewClinicalCase = auto()
        NewSevereCase = auto()
        DiseaseDeaths = auto()
        OpportunisticInfectionDeath = auto()
        NonDiseaseDeaths = auto()
        TBActivationSmearPos = auto()
        TBActivationSmearNeg = auto()
        TBActivationExtrapulm = auto()
        TBActivationPostRelapse = auto()
        TBPendingRelapse = auto()
        TBActivationPresymptomatic = auto()
        TestPositiveOnSmear = auto()
        ProviderOrdersTBTest = auto()
        TBTestPositive = auto()
        TBTestNegative = auto()
        TBTestDefault = auto()
        TBRestartHSB = auto()
        TBMDRTestPositive = auto()
        TBMDRTestNegative = auto()
        TBMDRTestDefault = auto()
        TBFailedDrugRegimen = auto()
        TBRelapseAfterDrugRegimen = auto()
        TBStartDrugRegimen = auto()
        TBStopDrugRegimen = auto()
        PropertyChange = auto()
        STIDebut = auto()
        StartedART = auto()
        StoppedART = auto()
        InterventionDisqualified = auto()
        HIVNewlyDiagnosed = auto()
        GaveBirth = auto()
        Pregnant = auto()
        Emigrating = auto()
        Immigrating = auto()
        HIVTestedNegative = auto()
        HIVTestedPositive = auto()
        #HIVPreARTToART = auto()
        #HIVNonPreARTToART = auto()
        TwelveWeeksPregnant = auto()
        FourteenWeeksPregnant = auto()
        SixWeeksOld = auto()
        EighteenMonthsOld = auto()
        STIPreEmigrating = auto()
        STIPostImmigrating = auto()
        STINewInfection = auto()
        NewExternalHIVInfection = auto()
        NodePropertyChange = auto()
        HappyBirthday = auto()
        EnteredRelationship = auto()
        ExitedRelationship = auto()
        FirstCoitalAct = auto()
        NewlySymptomatic = auto()
        SymptomaticCleared = auto()
        ExposureComplete = auto()
        SheddingComplete = auto()
        #PositiveResult = auto()
        #NegativeResult = auto()
        #Blackout = auto()
        #ReceivedTreatment = auto()
        #Recovered = auto()

    def __init__(self, file="./output/ReportEventRecorder.csv"):
        super().__init__(file)
        self.df[self.Column.Age_Year.name] = self.df[self.Column.Age.name]/365


class LineListReport(CsvOutput):
    class Column(Enum):
        TIME = auto()
        NODE = auto()
        AGE = auto()
        INFECTED = auto()
        MCWEIGHT = auto()
        MODACQUIRE = auto()

    def __init__(self, file="./output/ReportLineList.csv"):
        super().__init__(file)








