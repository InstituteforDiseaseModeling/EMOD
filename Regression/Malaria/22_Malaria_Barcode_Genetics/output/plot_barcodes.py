import os
import json
from collections import Counter
import matplotlib.pyplot as plt
import random

def strain2color(strain):

    antigenID=strain[0]
    geneticID=strain[1]

    B_MASK = 255
    G_MASK = 255<<8
    R_MASK = 255<<16

    r = (geneticID & R_MASK)>>16
    g = (geneticID & G_MASK)>>8
    b = geneticID & B_MASK

    if antigenID > 0:
        return [c/256. for c in [r,g,b]]
    else:
        return [0.5*(0.7+c/256.) for c in [r,g,b]] # mix 50-50 with light neutral color for more pleasing palette?

output_dir = '.'
annual_samples=1000.0
nyears=15 # note that this may be set to smaller time intervals, e.g. (73d * 5) * 3yr

plt.figure('Barcodes by Year',figsize=(15,10))
repeats={}
for year in range(nyears):
    plt.subplot(3, nyears//3, year+1)
    with open(os.path.join(output_dir, "MalariaSurveyJSONAnalyzer_Day0_%d.json" % year)) as f:
        f_json = json.loads(f.read())

    patients = f_json["patient_array"]
    print('There are %d unique patients reporting clinical incidents in this interval.' % len(patients))

    n_infections = []
    for p in patients:
        strains = [tuple(s) for s in p['strain_ids']]
        n_infections.append(len(set(strains)))

    multiplicity_of_infection = Counter(n_infections)
    print('Multiplicity of infection: ' + str(multiplicity_of_infection))
    sampling_rate = min(1.0, annual_samples/len(patients))

    barcode_counter=Counter()
    for p in patients:
        strains = [tuple(s) for s in p['strain_ids']]
        unique_strains = set(strains)
        if len(unique_strains) > 1:
            continue
        if random.random() > sampling_rate:
            continue
        s=unique_strains.pop()
        barcode_counter[s] += 1
    print('Barcodes: ' + str(barcode_counter))

    for k in repeats.keys():
        if k not in barcode_counter:
            repeats[k].append(0)

    for k,v in barcode_counter.items():
        if k not in repeats.keys():
            repeats[k]=[0]*year
        repeats[k].append(v)

    plt.title('Interval-%d' % year)
    plt.pie(barcode_counter.values(), labels=barcode_counter.keys(), colors=[strain2color(s) for s in barcode_counter.keys()])

xx,yy,ss,ll=[],[],[],[]
i=0
plt.figure('Repeat barcodes', figsize=(15,10))
for k,v in sorted(repeats.items(), reverse=True):
    if sum([n>0 for n in v]) <= 1:
        continue
    xx.extend(range(nyears))
    yy.extend([i]*nyears)
    ss.extend(v)
    ll.extend([str(k)])
    nonzero=[t for t, e in enumerate(v) if e != 0]
    plt.plot([nonzero[0],nonzero[-1]],[i]*2,'k-',alpha=0.2)
    plt.scatter(range(nyears), [i]*nyears, [10*sz for sz in v], color=strain2color(k), alpha=0.6, linewidth=0.1)
    i+=1

#plt.scatter(xx, yy, [10*sz for sz in ss], color='k', alpha=0.3, linewidth=0.1)
plt.ylim([-0.5, len(ll)-0.5])
plt.xlim([-0.5, nyears-0.5])
plt.gca().set_xticks(range(nyears))
plt.gca().set_yticks(range(len(ll)))
plt.gca().set_yticklabels(ll)

plt.tight_layout()
plt.show()