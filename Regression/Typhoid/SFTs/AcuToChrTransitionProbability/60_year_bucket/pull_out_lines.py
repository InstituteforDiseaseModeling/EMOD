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
            regex2 = "from acute"  #Not used, really
            for line in logfile:
                tryline = line.lower()
                if re.search(regex, tryline):  # and re.search( "individual_id = 10,", line ):
                    report_file.write(tryline)
                elif re.search(regex0, tryline) and re.search(regex2, tryline):
                    report_file.write(tryline)
                elif re.search(regex1, tryline) and re.search(regex2, tryline):
                    report_file.write(tryline)

if __name__ == "__main__":
    application("")
