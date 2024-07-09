#!/usr/bin/python
if __name__ == '__main__':
    import os
    from pathlib import Path
    import sys
    os.chdir(str(Path(sys.argv[0]).parent))
    sys.path.append( str(Path('../../shared_embedded_py_scripts').resolve().absolute()) )
"""
This is a script I used to verify issue: https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/3672. The ICP
core feature is tested under /regression/STI/SFTs/InterventionForCurrentPartners
"""

import json
import os.path as path
import dtk_test.dtk_sft as sft
import matplotlib.pyplot as plt
import pandas as pd
import math
import numpy as np
import dtk_STI_Support as sti_s

if __name__ == "__main__":
    df = sti_s.parse_relationship_start_report(report_path='output')
    relationships = {}
    age_map = {}
    for index, row in df.iterrows():
        time = row[sti_s.ReportHeader.rel_start_time]
        if time > 1:
            break
        f_id = row[sti_s.ReportHeader.b_ID]
        m_id = row[sti_s.ReportHeader.a_ID]
        age = row[sti_s.ReportHeader.a_age] * 365
        if m_id not in age_map:
            age_map[m_id] = (time, age)
        if f_id not in relationships:
            relationships[f_id] = m_id#[m_id]
        else:
            older_male = relationships[f_id]#[0]
            older_male_age = age_map[older_male][1] + time - age_map[older_male][0]
            if age > older_male_age:
                relationships[f_id] = m_id#[m_id]
            # elif age == older_male_age:
            #     relationships[f_id].append(m_id)
    print(f" {len(relationships)} female individuals in relationship at day 0.\n")
    report_df = pd.read_csv(os.path.join('output', 'ReportEventRecorder.csv'))
    print(f"{len(report_df)} events are broadcast.\n")

    male_ids = set(relationships.values())
    if len(male_ids) != len(report_df):
        print(f"BAD: expected {len(male_ids)} event got {len(report_df)} in ReportEventRecorder.csv.\n")
    else:
        print(f"GOOD: expected {len(male_ids)} event got {len(report_df)} in ReportEventRecorder.csv.\n")
        succeed = True
        for index, row in report_df.iterrows():
            id = row["Individual_ID"]
            if id not in male_ids:
                succeed = False
                print(f"BAD: male {id} should not receive event.\n")
            else:
                male_ids.remove(id)
        if succeed:
            print("GOOD: all male partners are expected to receive HIVSimpleDiagnostic intervention have received the "
                  "correct intervention.\n")
        else:
            if len(male_ids):
                print(f"BAD: these male partners should receive HIVSimpleDiagnostic intervention but they didn't: "
                      f"{male_ids}\n")