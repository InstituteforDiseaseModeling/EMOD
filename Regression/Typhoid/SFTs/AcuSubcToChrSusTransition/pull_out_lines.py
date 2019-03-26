#!/usr/bin/python

import re
import json
import math
import pdb
import matplotlib.pyplot as plt
import numpy as np


def application( report_file ):
    #print( "Post-processing: " + report_file )
    data = {}
    with open("filtered_lines.txt", "w") as report_file:
        with open("test.txt") as logfile:
            regex = "Time:"
            regex0 = "just went chronic"
            regex1 = "just recovered"
            regex2 = "larval_habitat ="  #Not used, really
            for line in logfile:
                if re.search(regex, line):  # and re.search( "individual_id = 10,", line ):
                    report_file.write(line)
                if re.search(regex1, line):
                    report_file.write(line)
                if re.search(regex0, line):
                    report_file.write(line)
                if re.search(regex2, line):
                    report_file.write(line)

if __name__ == "__main__":
    application("")
