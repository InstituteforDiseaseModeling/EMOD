#!/usr/bin/python

__author__ = 'clorton'


import buildMigrationFiles as bmf
import json
import os
import unittest

class TestLoadNodeIds(unittest.TestCase):

    def setUp(self):
        self.network = bmf.MigrationNetwork(8)
        pass

    def tearDown(self):
        pass

    def test_valid(self):
        ground_truth = [1487548419, 1487613955, 1487679488, 1487679489, 1487679490, 1487679491, 1487679492, 1487679493, 1487679494, 1487745027, 1487810563]
        (node_ids, reference) = self.network.loadDemographicsFile('unittests/valid_demographics.json')
        for index in range(len(node_ids)):
            self.assertEqual(node_ids[index], ground_truth[index], "Loaded id (%s) didn't match ground_truth (%s)" % (node_ids[index], ground_truth[index]))

    def test_non_json(self):
        with self.assertRaises(bmf.BuildException):
            self.network.loadDemographicsFile('unittests/non_json.txt')

    def test_missing_nodes(self):
        with self.assertRaises(bmf.BuildException):
            self.network.loadDemographicsFile('unittests/missing_node_array.json')

    def test_nodes_wrong_type(self):
        with self.assertRaises(bmf.BuildException):
            self.network.loadDemographicsFile('unittests/nodes_is_object.json')

    def test_missing_metadata(self):
        with self.assertRaises(bmf.BuildException):
            self.network.loadDemographicsFile('unittests/missing_metadata.json')

    def test_missing_idreference(self):
        with self.assertRaises(bmf.BuildException):
            self.network.loadDemographicsFile('unittests/missing_idreference.json')


class TestLoadMigrationRates(unittest.TestCase):

    def setUp(self):
        self.network = bmf.MigrationNetwork(8)
        self.network.loadDemographicsFile('unittests/valid_demographics.json')

    def tearDown(self):
        pass

    def test_valid(self):
        ground_truth = {
            1487548419: {1487613955: 0.15},
            1487613955: {1487548419: 0.10, 1487679491: 0.15},
            1487679488: {1487679489: 0.10},
            1487679489: {1487679488: 0.15, 1487679490: 0.10},
            1487679490: {1487679489: 0.15, 1487679491: 0.10},
            1487679491: {1487613955: 0.10, 1487679490: 0.15, 1487679492: 0.10, 1487745027: 0.15},
            1487679492: {1487679491: 0.15, 1487679493: 0.10},
            1487679493: {1487679492: 0.15, 1487679494: 0.10},
            1487679494: {1487679493: 0.15},
            1487745027: {1487679491: 0.10, 1487810563: 0.15},
            1487810563: {1487745027: 0.10}
        }
        self.network.loadMigrationRates('unittests/valid_migration.txt')
        links = self.network.links
        self.assertEqual(len(links), len(ground_truth), 'loadMigrationRates loaded incorrect number of links (%s), expected (%s)' % (len(links), len(ground_truth)))
        for node_id in ground_truth:
            self.assertIn(node_id, links, 'source node %s not found in loaded migration rates' % node_id)
            self.assertEqual(len(links[node_id]), len(ground_truth[node_id]), "number of links for node %s doesn't match ground truth (%s)" % (len(links[node_id]), len(ground_truth[node_id])))
            for destination_id in ground_truth[node_id]:
                self.assertIn(destination_id, links[node_id], 'link to %s not found in loaded migration rates' % destination_id)
                self.assertEqual(links[node_id][destination_id], ground_truth[node_id][destination_id], "loaded rate %s doesn't match ground truth (%s)" % (links[node_id][destination_id], ground_truth[node_id][destination_id]))

    def test_bad_source_node_id(self):
        with self.assertRaises(bmf.BuildException):
            links = self.network.loadMigrationRates('unittests/bad_source_node_id.txt')

    def test_bad_destination_node_id(self):
        with self.assertRaises(bmf.BuildException):
            links = self.network.loadMigrationRates('unittests/bad_destination_node_id.txt')

    def test_missing_destination_and_rate(self):
        with self.assertRaises(bmf.BuildException):
            links = self.network.loadMigrationRates('unittests/missing_destination_and_rate.txt')

    def test_missing_rate(self):
        with self.assertRaises(bmf.BuildException):
            links = self.network.loadMigrationRates('unittests/missing_rate.txt')

    def test_self_referential_link(self):
        with self.assertRaises(bmf.BuildException):
            links = self.network.loadMigrationRates('unittests/self_referential_link.txt')

    def test_multiply_defined_link(self):
        with self.assertRaises(bmf.BuildException):
            links = self.network.loadMigrationRates('unittests/multiply_defined_link.txt')


class TestValidateNetwork(unittest.TestCase):

    def setUp(self):
        self.network = bmf.MigrationNetwork(8)
        self.network.loadDemographicsFile('unittests/valid_demographics.json')

    def tearDown(self):
        pass

    def test_extra_links(self):
        self.network.links = self.network.loadMigrationRates('unittests/valid_migration.txt')
        self.network.link_count = 3
        self.assertFalse(self.network.validate(), 'migration file should have generated a warning')

    def test_island_node(self):
        self.network.links = self.network.loadMigrationRates('unittests/island_migration.txt')
        self.assertFalse(self.network.validate(), 'migration file should have generated a warning')

    def test_sink_node(self):
        self.network.links = self.network.loadMigrationRates('unittests/sink_migration.txt')
        self.assertFalse(self.network.validate(), 'migration file should have generated a warning')

    def test_source_node(self):
        self.network.links = self.network.loadMigrationRates('unittests/source_migration.txt')
        self.assertFalse(self.network.validate(), 'migration file should have generated a warning')


class TestWriteMigrationBinaryFile(unittest.TestCase):

    def setUp(self):
        self.network = bmf.MigrationNetwork(8)
        self.network.loadDemographicsFile('unittests/valid_demographics.json')
        self.network.links = self.network.loadMigrationRates('unittests/valid_migration.txt')
        self.filename = os.tempnam()

    def tearDown(self):
        os.remove(self.filename)

    def test_write_migration_binary_file(self):
        def getFileContents(filename):
            handle = open(filename)
            try:
                contents = handle.read()
            finally:
                handle.close()
            return contents

        self.network.writeMigrationBinaryFile(self.filename)
        gold = getFileContents('unittests/valid_migration.bin')
        test = getFileContents(self.filename)
        self.assertMultiLineEqual(test, gold, "Written binary file doesn't match expected.")


class TestWriteMigrationHeaderFile(unittest.TestCase):

    def setUp(self):
        self.network = bmf.MigrationNetwork(8)
        self.network.loadDemographicsFile('unittests/valid_demographics.json')
        self.network.loadMigrationRates('unittests/valid_migration.txt')
        self.filename = os.tempnam()

    def tearDown(self):
        os.remove(self.filename)

    def test_write_migration_header_file(self):
        def getJsonData(filename):
            handle = open(filename)
            try:
                json_data = json.load(handle)
            finally:
                handle.close()
            return json_data

        self.network.writeMigrationHeaderFile(self.filename)
        gold = getJsonData('unittests/valid_migration.bin.json')
        test = getJsonData(self.filename)
        self.assertEqual(test['Metadata']['IdReference'],    gold['Metadata']['IdReference'],    "IdReference doesn't match '%s' (expected '%s')" % (test['Metadata']['IdReference'], gold['Metadata']['IdReference']))
        self.assertEqual(test['Metadata']['NodeCount'],      gold['Metadata']['NodeCount'],      "NodeCount doesn't match '%s' (expected '%s')" % (test['Metadata']['NodeCount'], gold['Metadata']['NodeCount']))
        self.assertEqual(test['Metadata']['DatavalueCount'], gold['Metadata']['DatavalueCount'], "DatavalueCount doesn't match '%s' (expected '%s')" % (test['Metadata']['DatavalueCount'], gold['Metadata']['DatavalueCount']))
        self.assertEqual(test['NodeOffsets'],                gold['NodeOffsets'],                "NodeOffsets doesn't match '%s' (expected '%s')" % (test['NodeOffsets'], gold['NodeOffsets']))


class TestCanonicalTypeForMigration(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_uppercase(self):
        self.assertEqual(bmf.canonicalTypeForMigration('LOCAL'), 'LOCAL')

    def test_lowercase(self):
        self.assertEqual(bmf.canonicalTypeForMigration('regional'), 'REGIONAL')

    def test_mixedcase(self):
        self.assertEqual(bmf.canonicalTypeForMigration('sEa'), 'SEA')

    def test_partial(self):
        self.assertEqual(bmf.canonicalTypeForMigration('REG'), 'REGIONAL')

    def test_invalid(self):
        with self.assertRaises(bmf.BuildException):
            bmf.canonicalTypeForMigration('Shadowfax')

if __name__ == '__main__':
    unittest.main()