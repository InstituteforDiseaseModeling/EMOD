#!/usr/bin/python

import dtkFileTools as dft
import dtkFileSupport as support
import os
import tempfile
import unittest


class TestReadVersionOne(unittest.TestCase):

    def test_reading_uncompressed_file(self):

        jason_data = b'{"simulation":{"__class__":"SimulationPython","serializationMask":0,"nodes":[]}}'
        jason_text = '{"simulation":{"__class__":"SimulationPython","serializationMask":0,"nodes":[]}}'

        dtk = dft.read('test-data/simple.dtk')
        # noinspection SpellCheckingInspection
        self.assertEqual('clorton', dtk.author)
        self.assertEqual('notepad', dtk.tool)
        self.assertEqual(False, dtk.compressed)
        self.assertEqual(dft.NONE, dtk.compression)
        self.assertEqual(80, dtk.byte_count)
        self.assertEqual(1, dtk.chunk_count)
        self.assertEqual(80, dtk.chunk_sizes[0])
        self.assertEqual(jason_data, dtk.chunks[0])
        self.assertEqual(jason_text, dtk.contents[0])
        self.assertEqual('SimulationPython', dtk.simulation['__class__'])
        self.assertEqual(0, dtk.simulation.serializationMask)
        self.assertEqual(0, len(dtk.nodes))
        return

    def test_reading_compressed_file(self):
        dtk = dft.read('test-data/version1.dtk')
        self.assertEqual('', dtk.author)
        self.assertEqual('', dtk.tool)
        self.assertEqual(True, dtk.compressed)
        self.assertEqual(dft.SNAPPY, dtk.compression)
        self.assertEqual(1438033, dtk.byte_count)
        self.assertEqual(1, dtk.chunk_count)
        self.assertEqual(1438033, dtk.chunk_sizes[0])
        self.assertEqual(1, dtk.simulation.Run_Number)
        self.assertEqual(10001, dtk.simulation.individualHumanSuidGenerator.next_suid.id)
        self.assertEqual(0, dtk.simulation.sim_type)
        self.assertEqual(4, len(dtk.nodes))
        node = dtk.nodes[0]
        self.assertEqual(1, node.externalId)
        self.assertEqual(2500, len(node.individualHumans))
        human = node.individualHumans[0]
        self.assertEqual(1, human.cumulativeInfs)
        self.assertEqual(0, len(human.infections))
        self.assertEqual(0, human.infectiousness)
        self.assertEqual(9598.48, human.m_age)
        self.assertEqual(0, human.m_gender)
        self.assertEqual(False, human.m_is_infected)
        return

    def test_round_trip(self):
        source = dft.DtkFileV1()
        source.author = 'dtkFileTests'
        source.compression = dft.SNAPPY     # Choose something other than the default
        simulation = support.SerialObject({"simulation": {"enable_spatial_output": False, "Run_Number": 1, "enable_property_output": False, "infectionSuidGenerator": {"next_suid": {"id": 1}, "rank": 0, "numtasks": 1}, "__class__": "Simulation", "campaignFilename": "campaign.json", "nodes": [{"node": {"demographics_other": False, "birth_rate_sinusoidal_forcing_phase": 0, "sample_rate_20_plus": 0, "max_sampling_cell_pop": 0, "infectivity_boxcar_end_time": 0, "sample_rate_18mo_4yr": 0, "__class__": "Node", "infectivity_boxcar_start_time": 0, "demographics_gender": True, "individualHumans": [{"migration_is_destination_new_home": False, "home_node_id": {"id": 1}, "waiting_for_family_trip": False, "migration_will_return": True, "waypoints": [], "family_migration_type": 0, "Inf_Sample_Rate": 1, "migration_outbound": True, "m_mc_weight": 1, "migration_time_until_trip": 0, "StateChange": 0, "suid": {"id": 1}, "leave_on_family_trip": False, "interventions": {"drugVaccineReducedTransmit": 1, "drugVaccineReducedAcquire": 1, "__class__": "InterventionsContainer", "drugVaccineReducedMortality": 1, "interventions": []}, "migration_destination": {"id": 0}, "__class__": "IndividualHuman", "infectiousness": 0, "migration_mod": 1, "waypoints_trip_type": [], "m_age": 9588.48, "m_is_infected": False, "m_daily_mortality_rate": 0, "cumulativeInfs": 0, "family_migration_time_at_destination": 0, "family_migration_destination": {"id": 0}, "m_new_infection_state": 0, "migration_time_at_destination": 0, "family_migration_is_destination_new_home": False, "migration_type": 0, "max_waypoints": 0, "pregnancy_timer": 0, "family_migration_time_until_trip": 0, "is_pregnant": False, "susceptibility": {"trandecayoffset": 0, "age": 9588.48, "__class__": "Susceptibility", "acqdecayoffset": 0, "mod_mortality": 1, "mod_acquire": 1, "mod_transmit": 1, "mortdecayoffset": 0}, "is_on_family_trip": False, "infections": [], "above_poverty": 0, "Properties": [], "m_gender": 0}], "serializationMask": 3, "infectivity_sinusoidal_forcing_amplitude": 1, "birth_rate_boxcar_start_time": 0, "x_birth": 1, "sample_rate_immune": 0, "population_density_infectivity_correction": 0, "sample_rate_0_18mo": 0, "population_scaling": 0, "demographics_birth": False, "suid": {"id": 1}, "birth_rate_boxcar_forcing_amplitude": 1, "population_scaling_factor": 1, "birth_rate_boxcar_end_time": 0, "ind_sampling_type": 0, "vital_birth_time_dependence": 0, "externalId": 1, "age_initialization_distribution_type": 1, "infectivity_sinusoidal_forcing_phase": 0, "immune_threshold_for_downsampling": 0, "sample_rate_5_9": 0, "home_individual_ids": [{"key": 1, "value": {"id": 1}}], "population_density_c50": 0, "sample_rate_birth": 0, "birth_rate_sinusoidal_forcing_amplitude": 1, "infectivity_boxcar_forcing_amplitude": 1, "vital_birth": False, "maternal_transmission": False, "vital_birth_dependence": 1, "infectivity_scaling": 0, "animal_reservoir_type": 0, "zoonosis_rate": 0, "sample_rate_10_14": 0, "sample_rate_15_19": 0}, "suid": {"id": 1}}], "loadbalance_filename": "", "serializationMask": 3, "individualHumanSuidGenerator": {"next_suid": {"id": 1001}, "rank": 0, "numtasks": 1}, "sim_type": 0, "enable_default_report": True, "enable_event_report": False, "Ind_Sample_Rate": 1, "demographic_tracking": False}})
        source.objects.append(simulation)
        handle, filename = tempfile.mkstemp()
        os.close(handle)
        dft.write(source, filename)

        dest = dft.read(filename)
        self.assertEqual('dtkFileTests', dest.author)
        self.assertEqual(dft.SNAPPY, dest.compression)
        simulation = dest.simulation
        self.assertEqual(False, simulation.enable_spatial_output)
        self.assertEqual(1, simulation.Run_Number)
        self.assertEqual(1001, simulation.individualHumanSuidGenerator.next_suid.id)
        self.assertEqual(0, simulation.sim_type)
        self.assertEqual(True, simulation.enable_default_report)
        node = simulation.nodes[0].node     # Simulation nodes is a map<suid,node>
        self.assertEqual(1, node.externalId)
        self.assertEqual(False, node.demographics_other)
        self.assertEqual(True, node.demographics_gender)
        self.assertEqual(1, node.x_birth)
        individual = node.individualHumans[0]
        self.assertEqual(1, individual.m_mc_weight)
        self.assertEqual(1, individual.suid.id)
        self.assertEqual(9588.48, individual.m_age)
        self.assertEqual(0, individual.m_gender)
        os.remove(filename)
        return


class TestReadVersionTwo(unittest.TestCase):

    def test_reading_file(self):
        dtk = dft.read('test-data/version2.dtk')
        self.assertEqual('', dtk.author)
        self.assertEqual('', dtk.tool)
        self.assertEqual(True, dtk.compressed)
        self.assertEqual(dft.LZ4, dtk.compression)
        self.assertEqual(168431, dtk.byte_count)
        self.assertEqual(2, dtk.chunk_count)
        self.assertEqual(1584, dtk.chunk_sizes[0])
        self.assertEqual(166847, dtk.chunk_sizes[1])
        self.assertEqual(28, dtk.simulation.Run_Number)
        self.assertEqual(1001, dtk.simulation.individualHumanSuidGenerator.next_suid.id)
        self.assertEqual(2, dtk.simulation.sim_type)
        self.assertEqual(1, len(dtk.nodes))
        node = dtk.nodes[0]
        self.assertEqual(340461476, node.externalId)
        self.assertEqual(1000, len(node.individualHumans))
        human = node.individualHumans[0]
        self.assertEqual(0, human.cumulativeInfs)
        self.assertEqual(0, len(human.infections))
        self.assertEqual(0, human.infectiousness)
        self.assertEqual(7693.72, human.m_age)
        self.assertEqual(1, human.m_gender)
        self.assertEqual(False, human.m_is_infected)
        return

    def test_round_trip(self):

        source = dft.DtkFileV2()
        source.author = 'dtkFileTests'
        source.compression = dft.SNAPPY     # Choose something other than the default
        simulation = support.SerialObject({"simulation": {"enable_spatial_output": False, "Run_Number": 1, "enable_property_output": False, "infectionSuidGenerator": {"next_suid": {"id": 1}, "rank": 0, "numtasks": 1}, "__class__": "Simulation", "campaignFilename": "campaign.json", "nodes": [], "loadbalance_filename": "", "serializationMask": 3, "individualHumanSuidGenerator": {"next_suid": {"id": 1001}, "rank": 0, "numtasks": 1}, "sim_type": 0, "enable_default_report": True, "enable_event_report": False, "Ind_Sample_Rate": 1, "demographic_tracking": False}})
        node = support.SerialObject({"suid": {"id": 1}, "node": {"demographics_other": False, "birth_rate_sinusoidal_forcing_phase": 0, "sample_rate_20_plus": 0, "max_sampling_cell_pop": 0, "infectivity_boxcar_end_time": 0, "sample_rate_18mo_4yr": 0, "__class__": "Node", "infectivity_boxcar_start_time": 0, "demographics_gender": True, "individualHumans": [{"migration_is_destination_new_home": False, "home_node_id": {"id": 1}, "waiting_for_family_trip": False, "migration_will_return": True, "waypoints": [], "family_migration_type": 0, "Inf_Sample_Rate": 1, "migration_outbound": True, "m_mc_weight": 1, "migration_time_until_trip": 0, "StateChange": 0, "suid": {"id": 1}, "leave_on_family_trip": False, "interventions": {"drugVaccineReducedTransmit": 1, "drugVaccineReducedAcquire": 1, "__class__": "InterventionsContainer", "drugVaccineReducedMortality": 1, "interventions": []}, "migration_destination": {"id": 0}, "__class__": "IndividualHuman", "infectiousness": 0, "migration_mod": 1, "waypoints_trip_type": [], "m_age": 9588.48, "m_is_infected": False, "m_daily_mortality_rate": 0, "cumulativeInfs": 0, "family_migration_time_at_destination": 0, "family_migration_destination": {"id": 0}, "m_new_infection_state": 0, "migration_time_at_destination": 0, "family_migration_is_destination_new_home": False, "migration_type": 0, "max_waypoints": 0, "pregnancy_timer": 0, "family_migration_time_until_trip": 0, "is_pregnant": False, "susceptibility": {"trandecayoffset": 0, "age": 9588.48, "__class__": "Susceptibility", "acqdecayoffset": 0, "mod_mortality": 1, "mod_acquire": 1, "mod_transmit": 1, "mortdecayoffset": 0}, "is_on_family_trip": False, "infections": [], "above_poverty": 0, "Properties": [], "m_gender": 0}], "serializationMask": 3, "infectivity_sinusoidal_forcing_amplitude": 1, "birth_rate_boxcar_start_time": 0, "x_birth": 1, "sample_rate_immune": 0, "population_density_infectivity_correction": 0, "sample_rate_0_18mo": 0, "population_scaling": 0, "demographics_birth": False, "suid": {"id": 1}, "birth_rate_boxcar_forcing_amplitude": 1, "population_scaling_factor": 1, "birth_rate_boxcar_end_time": 0, "ind_sampling_type": 0, "vital_birth_time_dependence": 0, "externalId": 1, "age_initialization_distribution_type": 1, "infectivity_sinusoidal_forcing_phase": 0, "immune_threshold_for_downsampling": 0, "sample_rate_5_9": 0, "home_individual_ids": [{"key": 1, "value": {"id": 1}}], "population_density_c50": 0, "sample_rate_birth": 0, "birth_rate_sinusoidal_forcing_amplitude": 1, "infectivity_boxcar_forcing_amplitude": 1, "vital_birth": False, "maternal_transmission": False, "vital_birth_dependence": 1, "infectivity_scaling": 0, "animal_reservoir_type": 0, "zoonosis_rate": 0, "sample_rate_10_14": 0, "sample_rate_15_19": 0}})
        source.objects.append(simulation)
        source.objects.append(node)
        handle, filename = tempfile.mkstemp()
        os.close(handle)    # Do not need this, just the filename
        dft.write(source, filename)

        dest = dft.read(filename)
        self.assertEqual('dtkFileTests', dest.author)
        self.assertEqual(dft.SNAPPY, dest.compression)
        simulation = dest.simulation
        self.assertEqual(False, simulation.enable_spatial_output)
        self.assertEqual(1, simulation.Run_Number)
        self.assertEqual(1001, simulation.individualHumanSuidGenerator.next_suid.id)
        self.assertEqual(0, simulation.sim_type)
        self.assertEqual(True, simulation.enable_default_report)
        node = dest.nodes[0]
        self.assertEqual(1, node.externalId)
        self.assertEqual(False, node.demographics_other)
        self.assertEqual(True, node.demographics_gender)
        self.assertEqual(1, node.x_birth)
        individual = node.individualHumans[0]
        self.assertEqual(1, individual.m_mc_weight)
        self.assertEqual(1, individual.suid.id)
        self.assertEqual(9588.48, individual.m_age)
        self.assertEqual(0, individual.m_gender)
        os.remove(filename)

        return


class TestReadVersionThree(unittest.TestCase):

    def test_reading_file(self):
        dtk = dft.read('test-data/version3.dtk')
        self.assertEqual('', dtk.author)
        self.assertEqual('', dtk.tool)
        self.assertEqual(True, dtk.compressed)
        self.assertEqual(dft.LZ4, dtk.compression)
        self.assertEqual(177151, dtk.byte_count)
        self.assertEqual(2, dtk.chunk_count)
        self.assertEqual(1574, dtk.chunk_sizes[0])
        self.assertEqual(175577, dtk.chunk_sizes[1])
        self.assertEqual(27, dtk.simulation.Run_Number)
        self.assertEqual(1032, dtk.simulation.individualHumanSuidGenerator.next_suid.id)
        self.assertEqual(1, dtk.simulation.sim_type)
        self.assertEqual(1, len(dtk.nodes))
        node = dtk.nodes[0]
        self.assertEqual(340461476, node.externalId)
        self.assertEqual(1023, len(node.individualHumans))
        human = node.individualHumans[0]
        self.assertEqual(8, human.cumulativeInfs)
        self.assertEqual(5, len(human.infections))
        self.assertEqual(1, human.infectiousness)
        self.assertEqual(2457.41, human.m_age)
        self.assertEqual(1, human.m_gender)
        self.assertEqual(True, human.m_is_infected)
        return

    def test_round_trip(self):

        source = dft.DtkFileV3()
        source.author = 'dtkFileTests'
        source.compression = dft.SNAPPY     # Choose something other than the default
        simulation = support.SerialObject({"enable_spatial_output": False, "Run_Number": 1, "enable_property_output": False, "infectionSuidGenerator": {"next_suid": {"id": 1}, "rank": 0, "numtasks": 1}, "__class__": "Simulation", "campaignFilename": "campaign.json", "nodes": [], "loadbalance_filename": "", "serializationMask": 3, "individualHumanSuidGenerator": {"next_suid": {"id": 1001}, "rank": 0, "numtasks": 1}, "sim_type": 0, "enable_default_report": True, "enable_event_report": False, "Ind_Sample_Rate": 1, "demographic_tracking": False})
        node = support.SerialObject({"demographics_other": False, "birth_rate_sinusoidal_forcing_phase": 0, "sample_rate_20_plus": 0, "max_sampling_cell_pop": 0, "infectivity_boxcar_end_time": 0, "sample_rate_18mo_4yr": 0, "__class__": "Node", "infectivity_boxcar_start_time": 0, "demographics_gender": True, "individualHumans": [{"migration_is_destination_new_home": False, "home_node_id": {"id": 1}, "waiting_for_family_trip": False, "migration_will_return": True, "waypoints": [], "family_migration_type": 0, "Inf_Sample_Rate": 1, "migration_outbound": True, "m_mc_weight": 1, "migration_time_until_trip": 0, "StateChange": 0, "suid": {"id": 1}, "leave_on_family_trip": False, "interventions": {"drugVaccineReducedTransmit": 1, "drugVaccineReducedAcquire": 1, "__class__": "InterventionsContainer", "drugVaccineReducedMortality": 1, "interventions": []}, "migration_destination": {"id": 0}, "__class__": "IndividualHuman", "infectiousness": 0, "migration_mod": 1, "waypoints_trip_type": [], "m_age": 9588.48, "m_is_infected": False, "m_daily_mortality_rate": 0, "cumulativeInfs": 0, "family_migration_time_at_destination": 0, "family_migration_destination": {"id": 0}, "m_new_infection_state": 0, "migration_time_at_destination": 0, "family_migration_is_destination_new_home": False, "migration_type": 0, "max_waypoints": 0, "pregnancy_timer": 0, "family_migration_time_until_trip": 0, "is_pregnant": False, "susceptibility": {"trandecayoffset": 0, "age": 9588.48, "__class__": "Susceptibility", "acqdecayoffset": 0, "mod_mortality": 1, "mod_acquire": 1, "mod_transmit": 1, "mortdecayoffset": 0}, "is_on_family_trip": False, "infections": [], "above_poverty": 0, "Properties": [], "m_gender": 0}], "serializationMask": 3, "infectivity_sinusoidal_forcing_amplitude": 1, "birth_rate_boxcar_start_time": 0, "x_birth": 1, "sample_rate_immune": 0, "population_density_infectivity_correction": 0, "sample_rate_0_18mo": 0, "population_scaling": 0, "demographics_birth": False, "suid": {"id": 1}, "birth_rate_boxcar_forcing_amplitude": 1, "population_scaling_factor": 1, "birth_rate_boxcar_end_time": 0, "ind_sampling_type": 0, "vital_birth_time_dependence": 0, "externalId": 1, "age_initialization_distribution_type": 1, "infectivity_sinusoidal_forcing_phase": 0, "immune_threshold_for_downsampling": 0, "sample_rate_5_9": 0, "home_individual_ids": [{"key": 1, "value": {"id": 1}}], "population_density_c50": 0, "sample_rate_birth": 0, "birth_rate_sinusoidal_forcing_amplitude": 1, "infectivity_boxcar_forcing_amplitude": 1, "vital_birth": False, "maternal_transmission": False, "vital_birth_dependence": 1, "infectivity_scaling": 0, "animal_reservoir_type": 0, "zoonosis_rate": 0, "sample_rate_10_14": 0, "sample_rate_15_19": 0})
        source.objects.append(simulation)
        source.objects.append(node)
        handle, filename = tempfile.mkstemp()
        os.close(handle)    # Do not need this, just the filename
        dft.write(source, filename)

        dest = dft.read(filename)
        self.assertEqual('dtkFileTests', dest.author)
        self.assertEqual(dft.SNAPPY, dest.compression)
        simulation = dest.simulation
        self.assertEqual(False, simulation.enable_spatial_output)
        self.assertEqual(1, simulation.Run_Number)
        self.assertEqual(1001, simulation.individualHumanSuidGenerator.next_suid.id)
        self.assertEqual(0, simulation.sim_type)
        self.assertEqual(True, simulation.enable_default_report)
        node = dest.nodes[0]
        self.assertEqual(1, node.externalId)
        self.assertEqual(False, node.demographics_other)
        self.assertEqual(True, node.demographics_gender)
        self.assertEqual(1, node.x_birth)
        individual = node.individualHumans[0]
        self.assertEqual(1, individual.m_mc_weight)
        self.assertEqual(1, individual.suid.id)
        self.assertEqual(9588.48, individual.m_age)
        self.assertEqual(0, individual.m_gender)
        os.remove(filename)

        return


class TestReadVersionFour(unittest.TestCase):

    def test_reading_file(self):
        dtk = dft.read('test-data/version4.dtk')
        self.assertEqual('IDM', dtk.author)
        self.assertEqual('DTK', dtk.tool)
        self.assertEqual(True, dtk.compressed)
        self.assertEqual(dft.LZ4, dtk.compression)
        self.assertEqual(625105, dtk.byte_count)
        self.assertEqual(5, dtk.chunk_count)
        self.assertEqual(384, dtk.chunk_sizes[0])
        self.assertEqual(164210, dtk.chunk_sizes[1])
        self.assertEqual(152439, dtk.chunk_sizes[2])
        self.assertEqual(156570, dtk.chunk_sizes[3])
        self.assertEqual(151502, dtk.chunk_sizes[4])
        self.assertEqual(1, dtk.simulation.Run_Number)
        self.assertEqual(10001, dtk.simulation.individualHumanSuidGenerator.next_suid.id)
        self.assertEqual(0, dtk.simulation.sim_type)
        self.assertEqual(4, len(dtk.nodes))
        node = dtk.nodes[0]
        self.assertEqual(1, node.externalId)
        self.assertEqual(2500, len(node.individualHumans))
        human = node.individualHumans[0]
        self.assertEqual(1, human.cumulativeInfs)
        self.assertEqual(0, len(human.infections))
        self.assertEqual(0, human.infectiousness)
        self.assertEqual(9598.48, human.m_age)
        self.assertEqual(0, human.m_gender)
        self.assertEqual(False, human.m_is_infected)
        return

    def test_round_trip(self):

        source = dft.DtkFileV4()
        source.author = 'dtkFileTests'
        source.compression = dft.SNAPPY     # Choose something other than the default
        simulation = support.SerialObject({"enable_spatial_output": False, "Run_Number": 42, "enable_property_output": False, "infectionSuidGenerator": {"next_suid": {"id": 1}, "rank": 0, "numtasks": 1}, "__class__": "Simulation", "campaignFilename": "campaign.json", "nodes": [], "loadbalance_filename": "", "serializationMask": 3, "individualHumanSuidGenerator": {"next_suid": {"id": 1001}, "rank": 0, "numtasks": 1}, "sim_type": 1, "enable_default_report": True, "enable_event_report": False, "Ind_Sample_Rate": 1, "demographic_tracking": False})
        node = support.SerialObject({"demographics_other": False, "birth_rate_sinusoidal_forcing_phase": 0, "sample_rate_20_plus": 0, "max_sampling_cell_pop": 0, "infectivity_boxcar_end_time": 0, "sample_rate_18mo_4yr": 0, "__class__": "Node", "infectivity_boxcar_start_time": 0, "demographics_gender": True, "individualHumans": [{"migration_is_destination_new_home": False, "home_node_id": {"id": 1}, "waiting_for_family_trip": False, "migration_will_return": True, "waypoints": [], "family_migration_type": 0, "Inf_Sample_Rate": 1, "migration_outbound": True, "m_mc_weight": 11, "migration_time_until_trip": 0, "StateChange": 0, "suid": {"id": 1}, "leave_on_family_trip": False, "interventions": {"drugVaccineReducedTransmit": 1, "drugVaccineReducedAcquire": 1, "__class__": "InterventionsContainer", "drugVaccineReducedMortality": 1, "interventions": []}, "migration_destination": {"id": 0}, "__class__": "IndividualHuman", "infectiousness": 0, "migration_mod": 1, "waypoints_trip_type": [], "m_age": 9588.48, "m_is_infected": False, "m_daily_mortality_rate": 0, "cumulativeInfs": 0, "family_migration_time_at_destination": 0, "family_migration_destination": {"id": 0}, "m_new_infection_state": 0, "migration_time_at_destination": 0, "family_migration_is_destination_new_home": False, "migration_type": 0, "max_waypoints": 0, "pregnancy_timer": 0, "family_migration_time_until_trip": 0, "is_pregnant": False, "susceptibility": {"trandecayoffset": 0, "age": 9588.48, "__class__": "Susceptibility", "acqdecayoffset": 0, "mod_mortality": 1, "mod_acquire": 1, "mod_transmit": 1, "mortdecayoffset": 0}, "is_on_family_trip": False, "infections": [], "above_poverty": 0, "Properties": [], "m_gender": 0}], "serializationMask": 3, "infectivity_sinusoidal_forcing_amplitude": 1, "birth_rate_boxcar_start_time": 0, "x_birth": 2.718281828, "sample_rate_immune": 0, "population_density_infectivity_correction": 0, "sample_rate_0_18mo": 0, "population_scaling": 0, "demographics_birth": False, "suid": {"id": 1}, "birth_rate_boxcar_forcing_amplitude": 1, "population_scaling_factor": 1, "birth_rate_boxcar_end_time": 0, "ind_sampling_type": 0, "vital_birth_time_dependence": 0, "externalId": 314159265, "age_initialization_distribution_type": 1, "infectivity_sinusoidal_forcing_phase": 0, "immune_threshold_for_downsampling": 0, "sample_rate_5_9": 0, "home_individual_ids": [{"key": 1, "value": {"id": 1}}], "population_density_c50": 0, "sample_rate_birth": 0, "birth_rate_sinusoidal_forcing_amplitude": 1, "infectivity_boxcar_forcing_amplitude": 1, "vital_birth": False, "maternal_transmission": False, "vital_birth_dependence": 1, "infectivity_scaling": 0, "animal_reservoir_type": 0, "zoonosis_rate": 0, "sample_rate_10_14": 0, "sample_rate_15_19": 0})
        source.objects.append(simulation)
        source.objects.append(node)
        handle, filename = tempfile.mkstemp()
        os.close(handle)    # Do not need this, just the filename
        dft.write(source, filename)

        dest = dft.read(filename)
        self.assertEqual('dtkFileTests', dest.author)
        self.assertEqual(dft.SNAPPY, dest.compression)
        simulation = dest.simulation
        self.assertEqual(False, simulation.enable_spatial_output)
        self.assertEqual(42, simulation.Run_Number)
        self.assertEqual(1001, simulation.individualHumanSuidGenerator.next_suid.id)
        self.assertEqual(1, simulation.sim_type)
        self.assertEqual(True, simulation.enable_default_report)
        node = dest.nodes[0]
        self.assertEqual(314159265, node.externalId)
        self.assertEqual(False, node.demographics_other)
        self.assertEqual(True, node.demographics_gender)
        self.assertEqual(2.718281828, node.x_birth)
        individual = node.individualHumans[0]
        self.assertEqual(11, individual.m_mc_weight)
        self.assertEqual(1, individual.suid.id)
        self.assertEqual(9588.48, individual.m_age)
        self.assertEqual(0, individual.m_gender)
        os.remove(filename)

        return


class TestReadingSadPath(unittest.TestCase):

    def test_reading_wrong_magic_number(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/bad-magic.dtk')
        return

    def test_reading_negative_header_size(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/neg-hdr-size.dtk')
        return

    def test_reading_zero_header_size(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/zero-hdr-size.dtk')
        return

    def test_reading_invalid_header(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/bad-header.dtk')
        return

# Missing version is considered version 1. Is this okay?
#    def test_reading_missing_version(self):
#        with self.assertRaises(UserWarning):
#            dtk_file = dft.read('test-data/missing-version.dtk')
#        return

    def test_reading_negative_version(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/neg-version.dtk')
        return

    def test_reading_zero_version(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/zero-version.dtk')
        return

    def test_reading_unknown_version(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/future-version.dtk')
        return

    def test_reading_negative_chunk_size(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/neg-chunk-size.dtk')
        return

    def test_reading_zero_chunk_size(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/zero-chunk-size.dtk')
        return

    def test_reading_truncated_file(self):
        with self.assertRaises(UserWarning):
            dft.read('test-data/truncated.dtk')  # simulation and one node (truncated)
        return

    # Compression/data mismatch (false/LZ4)
    def test_engine_data_mismatch_a(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/none-lz4.dtk')       # hdr (NONE) vs. actual (LZ4)
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Compression/data mismatch (false/SNAPPY)
    def test_engine_data_mismatch_b(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/none-snappy.dtk')    # hdr (NONE) vs. actual (SNAPPY)
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Compression/data mismatch (true+LZ4/NONE)
    def test_engine_data_mismatch_c(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/lz4-none.dtk')       # hdr (LZ4) vs. actual (NONE)
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Compression/data mismatch (true+LZ4/SNAPPY)
    def test_engine_data_mismatch_d(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/lz4-snappy.dtk')     # hdr (LZ4) vs. actual (SNAPPY)
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Compression/data mismatch (true+SNAPPY/NONE)
    def test_engine_data_mismatch_e(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/snappy-none.dtk')    # hdr (SNAPPY) vs. actual (NONE)
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Compression/data mismatch (true+SNAPPY/LZ4)
    def test_engine_data_mismatch_f(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/snappy-lz4.dtk')     # hdr (SNAPPY) vs. actual (LZ4)
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Corrupted simulation chunk (uncompressed)
    def test_bad_sim_chunk_none(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/bad-sim-none.dtk')
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Corrupted simulation chunk (lz4)
    def test_bad_sim_chunk_lz4(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/bad-sim-lz4.dtk')
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Corrupted simulation chunk (snappy)
    def test_bad_sim_chunk_snappy(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/bad-sim-snappy.dtk')
            # Accessing the simulation field raises the exception
            print(dtk_file.simulation.individualHumanSuidGenerator.next_suid.id)
        return

    # Corrupted chunk (uncompressed)
    def test_bad_chunk_none(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/bad-chunk-none.dtk')
            # Accessing the node raises the exception
            print(dtk_file.nodes[0].externalId)
        return

    # Corrupted chunk (LZ4)
    def test_bad_chunk_lz4(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/bad-chunk-lz4.dtk')
            # Accessing the node raises the exception
            print(dtk_file.nodes[0].externalId)
        return

    # Corrupted chunk (SNAPPY)
    def test_bad_chunk_snappy(self):
        with self.assertRaises(UserWarning):
            dtk_file = dft.read('test-data/bad-chunk-snappy.dtk')
            # Accessing the node raises the exception
            print(dtk_file.nodes[0].externalId)
        return


class TestRegressions(unittest.TestCase):

    # https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/1268
    def test_1268(self):
        version_two = dft.read('test-data/version2.dtk')
        simulation = version_two.simulation
        self.assertTrue('nodes' not in simulation)
        version_three = dft.read('test-data/version3.dtk')
        simulation = version_three.simulation
        self.assertTrue('nodes' not in simulation)
        version_four = dft.read('test-data/version4.dtk')
        simulation = version_four.simulation
        self.assertTrue('nodes' not in simulation)

        return


if __name__ == '__main__':
    unittest.main()
