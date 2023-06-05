# -*- coding: utf-8 -*-
"""
Created on Mon Aug 24 09:17:49 2020

@author: NicolasWiest-Daesslé
"""

# Load an output CSV file to Check the Matrix conformance

import pandas as pd
import glob
#%%


path="C:/Users/arno/AppData/Local/WD/Checkout/databases/*.csv"
#filename="C:/Users/NicolasWiestDaessle/AppData/Local/WD/Checkout/databases/0ad1a33e6f6f7e6c71cf1ce9520931a3_cc2.csv"

for filename in glob.glob(path):
    

    df=pd.read_csv(filename)
    df=df[['Plate', 'Well', 'timepoint','fieldId','sliceId','channel']]
    if  len(df.Well.value_counts().unique()) == 1:
        print(f"{filename} Complete dataset")
    else:
        print(f"{filename} Incomplete dataset")
        counts=df.Well.value_counts()
        list_vals=counts.value_counts()
        print(f"expecting number of processed data {list_vals.index[0]}")
        for idx in range(1, len(list_vals)):
            print(counts[counts==list_vals.index[idx]])
            for x in counts[counts==list_vals.index[idx]].index:
                print(df[df.Well==x])
        
#     print(df.Well.value_counts().unique())
        
    
    # #%%
    #%%

