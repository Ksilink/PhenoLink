# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

#%%

core=set()
maped=dict()
started=0
f = open("C:/Users/NicolasWiestDaessle/AppData/Local/WD/Checkout/HashLogs.txt", "r")
for line in f:
    line=line.strip()
    if line.startswith("Started Core"):
        core.add(line.replace("Started Core ",""))
        started+=1
        continue
    if "->" in line:
        c,p=line.split('->')
        maped[p]=c
        continue
    if line.startswith("Finished"):
        h=line.replace("Finished ","")
        if h in maped:
            c=maped[h]
            core.remove(c)
            del maped[h]
        continue

    print(line)
    break
f.close()
print("Number of started processed    :",started)
print("Remaining accepted processes   :", len(maped))
print("Remaining unfinished processes :", len(core))



#%%