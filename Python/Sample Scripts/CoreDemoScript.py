#!/usr/bin/env python
# -*- coding: utf-8 -*-


#NAME
#    checkout - Checkout module, use this to access internal information of the checkout software exposed in the interface

#FUNCTIONS
#    addDataset(...)
#        add the specified data to the dataset

#    addPath(...)
#        Add a path to the loadable wellplate group

#    availableDatasets(...)
#        availableDatasets() -> returns the names of available dataset

#    commitDataset(...)
#        Commit the specified dataset to the database

#    dataPath(...)
#        get Data path of loadable wellplates

#    deletePath(...)
#        delete a path from the loadable wellplate group

#    getDataset(...)
#        getDataset(platename (string sequence or string), fields (string sequence or string)) -> retreive specified datasets

#    getLoadedImageFileNames(...)
#        returns a dictionnary of the loaded image filenames

#    getSelectedImageFileNames(...)
#        returns a dictionnary of the selected image filenames

#    loadWellplate(...)
#        load specified wellplates

#    loadableWellPlates(...)
#        get list of loadable wellplates

#    loadedWellPlates(...)
#        loadedWellPlates() -> returns the loaded well plate names


# Load first Wellplate among the available ones
checkout.loadWellplate(checkout.loadableWellPlates()[0])

# Select some wells from the First available wellplate (if  loaded)
#checkout.selectWells('AssayPlate_Greiner_#781956', ['B4', 'C4', 'C5'])

checkout.selectWells(checkout.loadedWellPlates()[0], ['B4', 'C4' ])

# Error: checkout.selectWells('Bobo', ['C5'])


# Display the selection
checkout.displaySelection()

