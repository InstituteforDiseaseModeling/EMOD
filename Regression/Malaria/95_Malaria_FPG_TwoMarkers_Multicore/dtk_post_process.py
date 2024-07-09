# dtk_post_process.py
# -----------------------------------------------------------------------------
# DMB 12/30/2020
# PURPOSE: This script is used to verify that the data created ReportMalariaGenetics,
# ReportNodeDemographicsMalariaGenetics, ReportVectorStatsMalariaGenetics, and
# MalariaSqlReport all match.
# -----------------------------------------------------------------------------

import sys, os, json, collections, struct, datetime
import pandas as pd
import sqlite3

"""
InsetChart Channels
- Adult Vectors
- Avg Num Infections
- Avg Num Vector Infs
- Complexity of Infection
- Daily Bites per Human
- Daily EIR
- Fever Prevalence
- Infected
- Infected and Infectious Vectors
- Infectious Vectors
- Mean Parasitemia
- New Infections
- New Vector Infections
- Statistical Population
- True Prevalence

ReportNodeDemographicsMalariaGenetics columns
- Time
- NodeID
- NumIndividuals
- NumInfected
- AvgParasiteDensity
- AvgGametocyteDensity
- AvgVariantFractionPfEMP1Major
- AvgNumInfections
- AvgInfectionCleardDuration
- NumInfectionsCleared
- AAAAAAAAAAAAAAAAAAAAAAAA
- TAAAAAAAAAAAAAAAAAAAAAAA
- AAAAAAAAAAAAAAAAAAAAAAAT
- TAAAAAAAAAAAAAAAAAAAAAAT
- OtherBarcodes
- T
- NoDrugResistance

ReportVectorStatsMalariaGenetics columns
- Time
- NodeID
- Population
- VectorPopulation
- STATE_INFECTIOUS
- STATE_INFECTED
- STATE_ADULT
- STATE_MALE
- STATE_IMMATURE
- STATE_LARVA
- STATE_EGG
- NewEggsCount
- IndoorBitesCount
- IndoorBitesCount-Infectious
- OutdoorBitesCount
- OutdoorBitesCount-Infectious
- NewAdults
- UnmatedAdults
- DiedBeforeFeeding
- DiedDuringFeedingIndoor
- DiedDuringFeedingOutdoor
- MigrationFromCountLocal
- MigrationFromCountRegional
- SillySkeeter_AvailableHabitat
- SillySkeeter_EggCrowdingCorrection
- NumInfectousBitesGiven
- NumInfectousBitesReceived
- InfectiousBitesGivenMinusReceived
- NumVectorsNone
- NumVectorsOnlyOocysts
- NumVectorsOnlySporozoites
- NumVectorsBothOocystsSporozoites
- NumBitesAdults
- NumBitesInfected
- NumBitesInfectious
- NumDiedAdults
- NumDiedInfected
- NumDiedInfectious
- NumParasiteCohortsOocysts
- NumParasiteCohortsSporozoites
- NumOocysts
- NumSporozoites
- NumInfectiousToAdult
- NumInfectiousToInfected
- AAAAAAAAAAAAAAAAAAAAAAAA
- TAAAAAAAAAAAAAAAAAAAAAAA
- AAAAAAAAAAAAAAAAAAAAAAAT
- TAAAAAAAAAAAAAAAAAAAAAAT
- OtherBarcodes
"""


def CompareValues(messages, var, exp, act):
    """
    Compare two values - My unit test like feature
    """
    success = True
    if (abs(exp - act) > 0.00001):
        messages.append(var + ": Expected " + str(exp) + " but got " + str(act))
        success = False
    return success


class InsetChart:
    """
    A class for reading an InsetChart.json file.
    """

    def __init__(self):
        self.fn = ""
        self.json_data = {}

    def Read(self, filename, messages):
        """
        Read the file given by filename, verify the channels all have the same length and that
        it matches the number of time steps defined in the header.
        """
        self.fn = filename

        with open(filename, 'r') as file:
            self.json_data = json.load(file)

        exp_num_channels = int(self.json_data["Header"]["Channels"])
        act_num_channels = len(self.json_data["Channels"])
        CompareValues(messages, "InsetChart:num channels", exp_num_channels, act_num_channels)

        exp_dt = 1
        act_dt = int(self.json_data["Header"]["Simulation_Timestep"])
        CompareValues(messages, "InsetChart:Simulation_Timestep", exp_dt, act_dt)

        exp_num_timesteps = int(self.json_data["Header"]["Timesteps"])
        for channel_name in self.json_data["Channels"]:
            act_num_timesteps = len(self.json_data["Channels"][channel_name]["Data"])
            var_name = "InsetChart:" + channel_name + " - num Timesteps"
            CompareValues(messages, var_name, exp_num_timesteps, act_num_timesteps)

    def GetChannel(self, channel_name):
        """
        Return a channel of the report as an array
        """
        return self.json_data["Channels"][channel_name]["Data"]


class ReportNodeDemographics:
    """
    A class for reading ReportNodeDemographicsMalariaGenetics
    """

    def __init__(self):
        self.fn = ""
        self.df = {}

    def Read(self, filename, messages):
        """
        Read the file specified by filename and put the data into an internal data frame
        """
        self.fn = filename

        with open(filename, 'r') as file:
            self.df = pd.read_csv(file)

    def GetInfectionsForBarcode(self, barcode):
        """
        Return a list of number of infections in people that have the given barcode by
        simulation time.  This is the number of infections, not the number of infected
        people.
        """
        return self.df.groupby(['Time'])[barcode].sum()

    def GetDataAsInsetChartChannel(self, channel_name):
        """
        Return a list of data per time step that should be the same as the given channel
        that is in the InsetChart report.
        """
        if channel_name == "Statistical Population":
            return self.df.groupby(['Time'])['NumIndividuals'].sum()
        elif channel_name == "Infected":
            num_infected = self.df.groupby(['Time'])['NumInfected'].sum()
            stat_pop = self.df.groupby(['Time'])['NumIndividuals'].sum()
            res = num_infected / stat_pop
            return res
        elif channel_name == "Avg Num Infections":
            avg_num_infections_per_node = self.df.groupby(['Time', 'NodeID'])['AvgNumInfections'].sum()
            num_infected_per_node = self.df.groupby(['Time', 'NodeID'])['NumInfected'].sum()

            mult = avg_num_infections_per_node * num_infected_per_node

            num_infections_per_timestep = mult.groupby(['Time']).sum()
            num_infected_per_timestep = num_infected_per_node.groupby(['Time']).sum()

            res = num_infections_per_timestep / num_infected_per_timestep
            return res
        elif channel_name == "Drug Resistant Fraction of Infected People":
            num_infected_people = self.df.groupby(['Time'])['NumInfected'].sum()
            num_people_with_resistance = self.df.groupby(['Time'])['T'].sum()
            drug_fraction = num_people_with_resistance / num_infected_people
            return drug_fraction
        elif channel_name == "HRP Deleted Fraction of Infected People":
            num_infected_people = self.df.groupby(['Time'])['NumInfected'].sum()
            num_people_without_hrp = self.df.groupby(['Time'])['G'].sum()
            without_hrp_fraction = num_people_without_hrp / num_infected_people
            return without_hrp_fraction

        return []


class ReportVectorStats:
    """
    A class for reading ReportVectorStatsMalariaGenetics
    """

    def __init__(self):
        self.fn = ""
        self.df = {}

    def Read(self, filename, messages):
        """
        Read the file specified by filename and put the data into an internal data frame
        """
        self.fn = filename

        with open(filename, 'r') as file:
            self.df = pd.read_csv(file)

    def GetDataAsInsetChartChannel(self, channel_name):
        """
        Return a list of data per time step that should be the same as the given channel
        that is in the InsetChart report.
        """
        if channel_name == "Statistical Population":
            return self.df.groupby(['Time'])['Population'].sum()

        elif channel_name == "Adult Vectors":
            adult_vectors = self.df.groupby(['Time'])['VectorPopulation'].mean()
            return adult_vectors

        elif channel_name == "Infectious Vectors":
            adult_vectors = self.df.groupby(['Time'])['VectorPopulation'].sum()
            infectious_vectors = self.df.groupby(['Time'])['STATE_INFECTIOUS'].sum()
            infectious_vectors_fraction = infectious_vectors / adult_vectors
            return infectious_vectors_fraction

        elif channel_name == "Infected and Infectious Vectors":
            adult_vectors = self.df.groupby(['Time'])['VectorPopulation'].sum()
            infected_vectors = self.df.groupby(['Time'])['STATE_INFECTED'].sum()
            infectious_vectors = self.df.groupby(['Time'])['STATE_INFECTIOUS'].sum()
            infectious_vectors_fraction = (infected_vectors + infectious_vectors) / adult_vectors
            return infectious_vectors_fraction

        elif channel_name == "Daily Bites per Human":
            humans = self.df.groupby(['Time'])['Population'].sum()
            indoor_bites = self.df.groupby(['Time'])['IndoorBitesCount'].sum()
            outdoor_bites = self.df.groupby(['Time'])['OutdoorBitesCount'].sum()
            bites_per_human = (indoor_bites + outdoor_bites) / humans
            return bites_per_human

        elif channel_name == "Daily EIR":
            humans = self.df.groupby(['Time'])['Population'].sum()
            indoor_bites = self.df.groupby(['Time'])['IndoorBitesCount-Infectious'].sum()
            outdoor_bites = self.df.groupby(['Time'])['OutdoorBitesCount-Infectious'].sum()
            eir = (indoor_bites + outdoor_bites) / humans
            return eir

        elif channel_name == "Avg Num Vector Infs":
            infected_vectors = self.df.groupby(['Time'])['STATE_INFECTED'].sum()
            infectious_vectors = self.df.groupby(['Time'])['STATE_INFECTIOUS'].sum()
            num_cohorts_oocyts = self.df.groupby(['Time'])['NumParasiteCohortsOocysts'].sum()
            num_cohorts_sporozoites = self.df.groupby(['Time'])['NumParasiteCohortsSporozoites'].sum()
            avg_num_infs = (num_cohorts_oocyts + num_cohorts_sporozoites) / (infected_vectors + infectious_vectors)
            return avg_num_infs

        return []


class MalariaSqlReport:
    """
    A class for reading MalariaSqlReport
    """

    def __init__(self):
        self.fn = ""
        self.conn = None
        self.cursor = None

    def Open(self, filename, messages):
        """
        Open the database specified by filename and prepare for queries
        """
        self.fn = filename

        self.conn = sqlite3.connect(self.fn)
        self.conn.row_factory = lambda cursor, row: row[0]  # makes the output lists of values instead of tuples
        self.cursor = self.conn.cursor()

    def Close(self):
        self.conn.close()

    def GetDataAsInsetChartChannel(self, channel_name):
        """
        Extract data from the database and format it into a list of values similar to what
        is in the corresponding InsetChart channel.
        """
        if channel_name == "Statistical Population":
            self.cursor.execute("SELECT COUNT(*) FROM Health GROUP BY SimTime")
            return self.cursor.fetchall()
        elif channel_name == "Infected":
            self.cursor.execute("SELECT COUNT(*) FROM Health GROUP BY SimTime")
            pops = self.cursor.fetchall()
            self.cursor.execute("SELECT SUM(Infected) " \
                                "FROM (SELECT Health.SimTime, 0 as Infected " \
                                "FROM Health " \
                                "GROUP BY Health.SimTime " \
                                "UNION " \
                                "SELECT SimTime, COUNT(*) AS Infected " \
                                "FROM (SELECT InfectionData.SimTime AS SimTime, Infections.HumanID AS HumanID " \
                                "FROM Infections INNER JOIN InfectionData ON InfectionData.InfectionID = Infections.InfectionID " \
                                "GROUP BY SimTime, HumanID " \
                                "ORDER BY SimTime) " \
                                "GROUP BY SimTime) " \
                                "GROUP BY SimTime")
            infs = self.cursor.fetchall()
            infected = [int(i) / int(p) for i, p in zip(infs, pops)]
            return infected

        elif channel_name == "New Infections":
            self.cursor.execute(
                "SELECT SUM(Infected) FROM (SELECT Health.SimTime, 0 as Infected FROM Health GROUP BY Health.SimTime " \
                "UNION " \
                "Select SimTimeCreated, COUNT(*) " \
                "FROM (SELECT Infections.SimTimeCreated, Infections.HumanID " \
                "FROM Infections GROUP BY Infections.SimTimeCreated,Infections.HumanID) GROUP BY SimTimeCreated) " \
                "GROUP BY SimTime")
            new_infs = self.cursor.fetchall()
            return new_infs

        return []

    def GetInfectionsForBarcode(self, barcode):
        """
        Return a list of number of infections in people that have the given barcode by
        simulation time.  This is the number of infections, not the number of infected
        people.
        """
        self.cursor.execute("SELECT SUM(Infected) " \
                            "FROM (SELECT Health.SimTime, 0 as Infected " \
                            "FROM Health GROUP BY Health.SimTime " \
                            "UNION " \
                            "SELECT SimTime, COUNT(*) " \
                            "FROM InfectionData " \
                            "INNER JOIN Infections ON Infections.InfectionID = InfectionData.InfectionID " \
                            "INNER JOIN ParasiteGenomes ON ParasiteGenomes.GenomeID = Infections.GenomeID " \
                            "WHERE ParasiteGenomes.Barcode = '" + barcode + "' " \
                                                                            "GROUP BY SimTime " \
                                                                            "ORDER BY SimTime " \
                                                                            ") " \
                                                                            "GROUP BY SimTime")
        infs = self.cursor.fetchall()
        return infs


def CompareArrays(messages, name, exp_data, act_data):
    """
    Compare to the two arrays to make sure the values are 'very' similar
    """

    messages.append("!!!!!!!!!!!!!!!!!! Compare " + name + " !!!!!!!!!!!!!!!!!!!!!!!")
    success = True

    exp_num = len(exp_data)
    act_num = len(act_data)
    success = CompareValues(messages, name + ":num", exp_num, act_num)
    if not success:
        return success

    i = 0
    for exp_val, act_val in zip(exp_data, act_data):
        # print(str(i)+"-"+str(exp_val)+" ? "+str(act_val))
        success = CompareValues(messages, name + ":val[" + str(i) + "]", exp_val, act_val)
        if not success:
            return success
        i += 1

    messages.append("!!!!!!!!!!!!!!!!!! PASSED - " + name + " !!!!!!!!!!!!!!!!!!!!!!!")

    return success


def ShowUsage():
    print('\nUsage: %s [output directory]' % os.path.basename(sys.argv[0]))


def application(output_path="output"):
    print("!!!!! Check genetic reports !!!!!")

    # Define the names of the files to be used in the test
    inset_chart_fn = os.path.join(output_path, "InsetChart.json")
    node_demog_fn = os.path.join(output_path, "ReportNodeDemographicsMalariaGenetics.csv")
    vectors_fn = os.path.join(output_path, "ReportVectorStatsMalariaGenetics.csv")
    # sql_db_fn = os.path.join(output_path, "SqlReportMalariaGenetics.db")

    messages = []

    # Open all of the files
    inset_chart = InsetChart()
    inset_chart.Read(inset_chart_fn, messages)

    node_demog = ReportNodeDemographics()
    node_demog.Read(node_demog_fn, messages)

    vector_stats = ReportVectorStats()
    vector_stats.Read(vectors_fn, messages)

    # sql_db = MalariaSqlReport()
    # sql_db.Open(sql_db_fn, messages)

    # Verify the reports have similar data
    inset_chart_pop = inset_chart.GetChannel("Statistical Population")
    node_demog_pop = node_demog.GetDataAsInsetChartChannel("Statistical Population")
    vector_stats_pop = vector_stats.GetDataAsInsetChartChannel("Statistical Population")
    # sql_db_pop = sql_db.GetDataAsInsetChartChannel("Statistical Population")
    CompareArrays(messages, "InsetChart vs NodeDemog:Statistical Population", inset_chart_pop, node_demog_pop)
    CompareArrays(messages, "InsetChart vs VectorStats:Statistical Population", inset_chart_pop, vector_stats_pop)
    # CompareArrays(messages, "InsetChart vs SqlDB:Statistical Population", inset_chart_pop, sql_db_pop)

    inset_chart_inf = inset_chart.GetChannel("Infected")
    node_demog_inf = node_demog.GetDataAsInsetChartChannel("Infected")
    # sql_db_inf = sql_db.GetDataAsInsetChartChannel("Infected")
    CompareArrays(messages, "InsetChart vs NodeDemog:Infected", inset_chart_inf, node_demog_inf)
    # CompareArrays(messages, "InsetChart vs SqlDB:Infected", inset_chart_inf, sql_db_inf)

    inset_chart_new_inf = inset_chart.GetChannel("New Infections")
    # sql_db_new_inf = sql_db.GetDataAsInsetChartChannel("New Infections")
    # CompareArrays(messages, "InsetChart vs SqlDB:New Infections", inset_chart_new_inf, sql_db_new_inf)

    inset_chart_inf = inset_chart.GetChannel("Avg Num Infections")
    node_demog_inf = node_demog.GetDataAsInsetChartChannel("Avg Num Infections")
    CompareArrays(messages, "InsetChart vs NodeDemog:Avg Num Infections", inset_chart_inf, node_demog_inf)

    inset_chart_av = inset_chart.GetChannel("Adult Vectors")
    vector_stats_av = vector_stats.GetDataAsInsetChartChannel("Adult Vectors")
    CompareArrays(messages, "InsetChart vs VectorStats:Adult Vectors", inset_chart_av, vector_stats_av)

    inset_chart_iv = inset_chart.GetChannel("Infectious Vectors")
    vector_stats_iv = vector_stats.GetDataAsInsetChartChannel("Infectious Vectors")
    CompareArrays(messages, "InsetChart vs VectorStats:Infectious Vectors", inset_chart_iv, vector_stats_iv)

    inset_chart_in_iv = inset_chart.GetChannel("Infected and Infectious Vectors")
    vector_stats_in_iv = vector_stats.GetDataAsInsetChartChannel("Infected and Infectious Vectors")
    CompareArrays(messages, "InsetChart vs VectorStats:Infected and Infectious Vectors", inset_chart_in_iv,
                  vector_stats_in_iv)

    inset_chart_dbh = inset_chart.GetChannel("Daily Bites per Human")
    vector_stats_dbh = vector_stats.GetDataAsInsetChartChannel("Daily Bites per Human")
    CompareArrays(messages, "InsetChart vs VectorStats:Daily Bites per Human", inset_chart_dbh, vector_stats_dbh)

    inset_chart_eir = inset_chart.GetChannel("Daily EIR")
    vector_stats_eir = vector_stats.GetDataAsInsetChartChannel("Daily EIR")
    CompareArrays(messages, "InsetChart vs VectorStats:Daily EIR", inset_chart_eir, vector_stats_eir)

    inset_chart_nvi = inset_chart.GetChannel("Avg Num Vector Infs")
    vector_stats_nvi = vector_stats.GetDataAsInsetChartChannel("Avg Num Vector Infs")
    CompareArrays(messages, "InsetChart vs VectorStats:Avg Num Vector Infs", inset_chart_nvi, vector_stats_nvi)

    # node_demog_barcode_infs_AA = node_demog.GetInfectionsForBarcode("AAAAAAAAAAAAAAAAAAAAAAAA")
    # sql_db_barcode_infs_AA = sql_db.GetInfectionsForBarcode("AAAAAAAAAAAAAAAAAAAAAAAA")
    # CompareArrays(messages, "NodeDemog vs SqlDB:Barcode-AA", node_demog_barcode_infs_AA, sql_db_barcode_infs_AA)

    # node_demog_barcode_infs_TA = node_demog.GetInfectionsForBarcode("TAAAAAAAAAAAAAAAAAAAAAAA")
    # sql_db_barcode_infs_TA = sql_db.GetInfectionsForBarcode("TAAAAAAAAAAAAAAAAAAAAAAA")
    # CompareArrays(messages, "NodeDemog vs SqlDB:Barcode-TA", node_demog_barcode_infs_TA, sql_db_barcode_infs_TA)
    #
    # node_demog_barcode_infs_AT = node_demog.GetInfectionsForBarcode("AAAAAAAAAAAAAAAAAAAAAAAT")
    # sql_db_barcode_infs_AT = sql_db.GetInfectionsForBarcode("AAAAAAAAAAAAAAAAAAAAAAAT")
    # CompareArrays(messages, "NodeDemog vs SqlDB:Barcode-AT", node_demog_barcode_infs_AT, sql_db_barcode_infs_AT)
    #
    # node_demog_barcode_infs_TT = node_demog.GetInfectionsForBarcode("TAAAAAAAAAAAAAAAAAAAAAAT")
    # sql_db_barcode_infs_TT = sql_db.GetInfectionsForBarcode("TAAAAAAAAAAAAAAAAAAAAAAT")
    # CompareArrays(messages, "NodeDemog vs SqlDB:Barcode-TT", node_demog_barcode_infs_TT, sql_db_barcode_infs_TT)

    inset_drug_resistance = inset_chart.GetChannel("Drug Resistant Fraction of Infected People")
    node_demog_drug_resistance = node_demog.GetDataAsInsetChartChannel("Drug Resistant Fraction of Infected People")
    CompareArrays(messages, "InsetChart vs NodeDemog-Drug Resistant Fraction", inset_drug_resistance,
                  node_demog_drug_resistance)

    inset_hrp_deleted = inset_chart.GetChannel("HRP Deleted Fraction of Infected People")
    node_demog_hrp_deleted = node_demog.GetDataAsInsetChartChannel("HRP Deleted Fraction of Infected People")
    CompareArrays(messages, "InsetChart vs NodeDemog-HRP Fraction", inset_hrp_deleted, node_demog_hrp_deleted)

    # sql_db.Close()

    # Create a file with the results of the test
    output = {}
    output["messages"] = []
    for line in messages:
        output["messages"].append(line)

    report_fn = os.path.join(output_path, "report_sync_check.json")
    with open(report_fn, 'w') as report_file:
        json.dump(output, report_file, indent=4)

    for line in messages:
        print(line)

    print("!!!!! Done checking reports !!!!!")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        ShowUsage()
        exit(0)

    application(sys.argv[1])

# ---------------------------------------------------------------------------------------
# ---------------------------------------------------------------------------------------
