# -*- coding: utf-8 -*-
"""
Spyder Editor

This is a temporary script file.
"""

#%%

def prRed(skk): print("\033[91m {}\033[00m" .format(skk)) 
def prGreen(skk): print("\033[92m {}\033[00m" .format(skk)) 
def prYellow(skk): print("\033[93m {}\033[00m" .format(skk)) 
def prLightPurple(skk): print("\033[94m {}\033[00m" .format(skk)) 
def prPurple(skk): print("\033[95m {}\033[00m" .format(skk)) 
def prCyan(skk): print("\033[96m {}\033[00m" .format(skk)) 
def prLightGray(skk): print("\033[97m {}\033[00m" .format(skk)) 
def prBlack(skk): print("\033[98m {}\033[00m" .format(skk)) 

core=set()
maped=dict()
started=0
f = open("C:/Users/arno/AppData/Local/WD/Checkout/HashLogs.txt", "r")
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
if (len(maped) > 0 or len(core) > 0):
    prRed("KO")
else:
    prGreen("OK")


#%%