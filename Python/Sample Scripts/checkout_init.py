import pickle
import io
import numpy as np


class CheckoutFake:
    def __init__(self):
        ## Setup the class for handling the scripting functions of CheckoutCore
        self._availableDatasets = {'AssayPlate_Greiner_#781956': ['test_Basic_Otsu_Threshold_Thresholded_Pixel_Area',
                                                                  'test_Basic_Otsu_Threshold_Threshold_Isolated_Area_Count',
                                                                  'test_Basic_Otsu_Threshold_Value_of_computed_Otsu_Threshold',
                                                                  'test2_Basic_Threshold_Thresholded_Pixel_Area',
                                                                  'test2_Basic_Threshold_Threshold_Isolated_Area_Count']}

        self._loadableWellPlate = ['C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/MeasurementDetail.mrf',
                                   'C:/TEMP/build/Data/Assay Plate - Copie/Plate Bob/MeasurementDetail.mrf',
                                   'C:/TEMP/build/Data/Forged/Forged/MeasurementDetail.mrf',
                                   'C:/TEMP/build/Data/Forged - Copie/MeasurementDetail.mrf',
                                   'C:/TEMP/build/Data/Time course CHX_Test prog time course-2_20141016_140819/AssayPlate_BD_#356663/MeasurementDetail.mrf']
        self._loadedWellPlates = list()
        self._dataPath = ['C:/TEMP/build/Data/']

        # Simulate only B4 selection available
        self._selected = False
        self._selectedFileName = {b'AssayPlate_Greiner_#781956': {b'B4': {1: {1: {1: {
        1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F001L01A01Z01C01.tif',
        2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F001L01A02Z01C02.tif'}}},
                                                                          2: {1: {1: {
                                                                          1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F002L01A01Z01C01.tif',
                                                                          2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F002L01A02Z01C02.tif'}}},
                                                                          3: {1: {1: {
                                                                          1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F003L01A01Z01C01.tif',
                                                                          2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F003L01A02Z01C02.tif'}}},
                                                                          4: {1: {1: {
                                                                          1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F004L01A01Z01C01.tif',
                                                                          2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F004L01A02Z01C02.tif'}}}}}}
        self._loadedFileNames = {b'AssayPlate_Greiner_#781956': {b'F5': {1: {1: {1: {
        1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F05_T0001F001L01A01Z01C01.tif',
        2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'M5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'J5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'M4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_M04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'N4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'J4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_J04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'C5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'G5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'G4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_G04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'I4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'O4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'H5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'K5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'D4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'E4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'C4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_C04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'L5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'D5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_D05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'E5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_E05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'K4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_K04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'N5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_N05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'B4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'H4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_H04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'F4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_F04_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'I5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_I05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'O5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_O05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'B5': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B05_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B05_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B05_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B05_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B05_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B05_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B05_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_B05_T0001F004L01A02Z01C02.tif'}}}},
                                                                 b'L4': {1: {1: {1: {
                                                                 1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L04_T0001F001L01A01Z01C01.tif',
                                                                 2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L04_T0001F001L01A02Z01C02.tif'}}},
                                                                         2: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L04_T0001F002L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L04_T0001F002L01A02Z01C02.tif'}}},
                                                                         3: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L04_T0001F003L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L04_T0001F003L01A02Z01C02.tif'}}},
                                                                         4: {1: {1: {
                                                                         1: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L04_T0001F004L01A01Z01C01.tif',
                                                                         2: b'C:/TEMP/build/Data/Assay Plate/AssayPlate_Greiner_#781956/AssayPlate_Greiner_#781956_L04_T0001F004L01A02Z01C02.tif'}}}}}}

    def loadedWellPlates(self):
        return self._loadedWellPlates

    def selectedWellPlates(self):
        return self._availableDatasets[0]

    def availableDatasets(self):
        return self._availableDatasets

    def getDataset(self, str, data):
        if not 'AssayPlate_Greiner_#781956' in str and not 'test_Basic_Otsu_Threshold_Thresholded_Pixel_Area' in data:
            raise 'Error accessing data'

        r = {
        'activeRows': [False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, True, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False, False, False, False, False, False, False, False, False, False, False,
                       False, False, False, False],
        'Data': np.array([[0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 5513., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
                          [0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 2809., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 2118., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
                          [0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 2111., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
                          [0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 2363.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
                          [0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 2809., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
                          [0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 3316., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
                          [0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           2977., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.,
                           0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
                          [0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000, 0.00000000e+000,
                           8.32733897e-306, 5.13064630e+173, 5.44132692e+174, 5.44136466e+174, 7.29114165e-302,
                           4.94951870e+173 - 9.97514076e-177, 2.14044442e-308, 2.14044442e-308, 5.98214097e+197,
                           2.14033113e-308, 1.37426567e-048, 1.37359775e-048, 5.98214217e+197, 1.08194217e-305,
                           5.13909998e+173, 5.44132692e+174, 6.57819254e+198, 3.35173491e-309, 5.44132692e+174,
                           1.58913161e-234, 2.32251844e-186, 2.06958521e-287, 5.98214097e+197, 2.14033119e-308,
                           1.37426567e-048, 1.37359775e-048, 5.98214217e+197, 1.52306307e-305, 5.14755365e+173,
                           5.44132692e+174, 6.57816973e+198, 3.35173491e-309, 5.44132692e+174, 1.58913161e-234,
                           8.25124304e-201, 1.96106375e-309, 1.57274931e-234, 2.41846548e-308, 2.14044442e-308,
                           1.95033591e-305, 5.98214097e+197, 2.02147961e-305, 5.15600733e+173, 5.44132692e+174,
                           6.57816061e+198, 3.35173491e-309, 5.44132692e+174, 1.58913161e-234, 6.82526827e-225,
                           1.96106375e-309, 1.57275771e-234, 2.41846554e-308, 2.14044442e-308,
                           1.96220952e-309, - 5.93474075e-066, 2.76143400e-305, 1.37359775e-048, 1.37693870e-048,
                           5.98652211e+197, 3.35173491e-309, 5.44132692e+174, 1.58913161e-234, 1.58913161e-234,
                           1.96106375e-309, 1.57276611e-234, 2.41846560e-308, 2.14044442e-308,
                           1.96220951e-309, - 3.08149358e-032, 3.75826709e-305, 1.37359775e-048, 1.37593642e-048,
                           4.77831384e-297, 1.37359748e-048, 5.44133446e+174, 5.44132692e+174, 6.57895447e+198,
                           1.96106375e-309, 1.57277451e-234, 2.41846565e-308, 2.14044442e-308,
                           1.96220951e-309, - 1.60000294e+002, 4.95324909e-305, 1.37359775e-048, 1.37426594e-048,
                           4.77831384e-297, 1.37359748e-048, 5.44133446e+174, 5.44132692e+174, 7.29112830e-302,
                           5.18378270e+173, 6.75505115e-225, 1.58913161e-234 - 1.28102045e+031,
                           1.96106400e-309, - 8.30769025e+035, 6.94691526e-305, 1.37359775e-048, 1.37359775e-048,
                           4.77831384e-297, 1.37359748e-048, 5.44133446e+174, 5.44132692e+174, 7.29112830e-302,
                           5.19223637e+173, 6.75508722e-225, 1.58913161e-234, 7.35264125e-302,
                           4.94951870e+173, - 4.35908657e+069, 2.14044442e-308, 3.53111558e-308, 6.13422306e-309,
                           4.77831384e-297, 1.37359748e-048, 5.44133446e+174, 5.44132692e+174, 7.29112830e-302,
                           5.20069005e+173, 6.75512330e-225, 1.58913161e-234, 7.35264116e-302,
                           4.94951870e+173, - 2.26336715e+103, 2.14044442e-308, 3.11391424e-308, 5.98214097e+197,
                           2.14033161e-308, 1.37426567e-048, 1.37359775e-048, 1.44075096e-048, 7.29112829e-302,
                           4.96398713e+173, 6.75411848e-225, 1.58913161e-234, 7.35264113e-302, 4.94951870e+173,
                           2.93143040e-215, 2.14044442e-308, 2.41857866e-308, 5.98214097e+197, 2.14033000e-308,
                           1.37426566e-048, 1.37359775e-048, 5.98214217e+197, 3.25286678e-308, 4.97606480e+173,
                           5.44132692e+174, 5.44136466e+174, 7.29114165e-302, 4.94951870e+173 - 2.73624299e+127,
                           2.14044442e-308, 2.14044442e-308, 5.98214097e+197, 2.14033165e-308, 1.37426567e-048,
                           1.37359775e-048, 5.98214217e+197, 1.67414558e-304, 5.21518306e+173, 5.44132692e+174,
                           6.57819254e+198, 3.35173491e-309, 5.44132692e+174, 1.58913161e-234, 2.32251844e-186,
                           2.06958552e-287, 5.98214097e+197, 2.14033171e-308, 1.37426567e-048, 1.37359775e-048,
                           5.98214217e+197, 2.32297712e-304, 5.22363674e+173, 5.44132692e+174, 6.57816973e+198,
                           3.35173491e-309, 5.44132692e+174, 1.58913161e-234, 8.25124304e-201, 1.96106375e-309,
                           1.57282490e-234, 2.41846600e-308, 2.14044442e-308, 3.00661367e-304, 5.98214097e+197,
                           3.12044360e-304, 5.23209041e+173, 5.44132692e+174, 6.57816061e+198, 3.35173491e-309,
                           5.44132692e+174, 1.58913161e-234, 6.82526827e-225, 1.96106375e-309, 1.57283330e-234,
                           2.41846606e-308, 2.14044442e-308, 1.96220952e-309 - 1.62793620e+238, 4.19044683e-304,
                           1.37359775e-048, 1.37693870e-048, 5.98652211e+197, 3.35173491e-309, 5.44132692e+174,
                           1.58913161e-234, 1.58913161e-234, 1.96106375e-309, 1.57284170e-234, 2.41846612e-308,
                           2.14044442e-308, 1.96220951e-309 - 8.45272804e+271, 5.78537977e-304, 1.37359775e-048,
                           1.37593642e-048, 4.77831384e-297, 1.37359748e-048, 5.44133446e+174, 5.44132692e+174,
                           6.57924190e+198, 1.96106375e-309, 1.57285009e-234, 2.41846618e-308, 2.14044442e-308,
                           1.96220951e-309 - 4.38890733e+305, 7.46950341e-304, 1.37359775e-048, 1.37426594e-048,
                           4.77831384e-297, 1.37359748e-048, 5.44133446e+174, 5.44132692e+174, 7.29112830e-302,
                           5.25986578e+173, 6.75537579e-225, 1.58913161e-234, 1.08732800e-282, 1.96106400e-309,
                           7.05155350e-278, 1.06593693e-303, 1.37359775e-048, 1.37359775e-048, 4.77831384e-297, 1.37359748e-048, 5.44133446e+174, 5.44132692e+174,
                           7.29112830e-302, 5.26831946e+173, 6.75541186e-225, 1.58913161e-234, 7.35264125e-302,
                           4.94951870e+173, 3.69998535e-244, 2.14044442e-308, 3.53111558e-308, 3.80924983e-308,
                           4.95550986e+173, 2.03212242e-110, 2.36890564e-308, 1.44842210e-282, 4.95191045e+173,
                           8.88725834e-309, 4.94705619e+173, 1.30592721e-308, 1.23738600e-282, 5.72969810e+250,
                           6.73606386e-310, 6.70788740e-302, 4.95188686e+173, 7.46942522e-309, 2.05227017e-287,
                           5.98214097e+197, 2.14032995e-308, 1.37426566e-048, 1.37359775e-048, 5.98214217e+197,
                           2.55753120e-308, 4.97002646e+173, 5.44132692e+174, 6.57816973e+198, 3.35173491e-309,
                           5.44132692e+174, 1.58913161e-234, 8.25124304e-201, 1.96106375e-309, 1.57257415e-234,
                           2.41846427e-308, 2.14044442e-308, 3.67018270e-308, 6.74752230e-310, 8.08635618e-172,
                           5.98214097e+197, 3.94820236e-308, 4.98210314e+173, 5.44132692e+174, 6.57816061e+198,
                           3.35173491e-309, 5.44132692e+174, 1.58913161e-234, 6.82526827e-225, 1.96106375e-309,
                           1.57258495e-234, 2.41846434e-308, 2.14044442e-308, 1.96220952e-309, 5.12534215e-143,
                           5.39342577e-308, 1.37359775e-048, 1.37693870e-048, 5.98652211e+197, 3.35173491e-309,
                           5.44132692e+174, 1.58913161e-234, 1.58913161e-234, 1.96106375e-309, 1.57259335e-234,
                           2.41846440e-308, 2.14044442e-308, 1.96220951e-309, 2.66122979e-109, 7.34036540e-308,
                           1.37359775e-048, 1.37593642e-048, 4.77831384e-297, 1.37359747e-048, 5.44133446e+174,
                           5.44132692e+174, 6.57829748e+198, 1.96106375e-309, 1.57260174e-234, 2.41846446e-308,
                           2.14044442e-308, 1.96220951e-309, 1.38178951e-075, 9.67431462e-308, 1.37359775e-048,
                           1.37426594e-048, 4.77831384e-297, 1.37359747e-048, 5.44133446e+174, 5.44132692e+174,
                           7.29112830e-302, 5.00987851e+173, 6.75430913e-225, 1.58913161e-234, 1.10631085e-046,
                           1.96106400e-309, 7.17466133e-042, 1.35681939e-307, 1.37359775e-048, 1.37359775e-048,
                           4.77831384e-297, 1.37359747e-048, 5.44133446e+174, 5.44132692e+174, 7.29112830e-302,
                           5.01833219e+173, 6.75434520e-225, 1.58913161e-234, 7.35264125e-302, 4.94951870e+173,
                           3.76458064e-008, 2.14044442e-308, 3.53111558e-308, 6.13422301e-309, 4.77831384e-297,
                           1.37359747e-048, 5.44133446e+174, 5.44132692e+174, 7.29112830e-302, 5.02678586e+173,
                           6.75438127e-225, 1.58913161e-234, 7.35264116e-302, 4.94951870e+173, 1.95468202e+026,
                           2.14044442e-308, 3.11391424e-308, 5.98214097e+197, 2.14033042e-308, 1.37426566e-048,
                           1.37359775e-048, 1.39264120e-048, 7.29112829e-302, 5.03523954e+173, 6.75441735e-225,
                           1.58913161e-234, 7.35264113e-302, 4.94951870e+173, 1.01492893e+060, 2.14044442e-308,
                           2.41857866e-308, 5.98214097e+197, 2.14033048e-308, 1.37426566e-048, 1.37359775e-048,
                           5.98214217e+197, 3.49232297e-307, 5.04369421e+173, 5.44132692e+174, 5.44136466e+174,
                           7.29114165e-302, 4.94951870e+173, 5.26981231e+093, 2.14044442e-308, 2.14044442e-308,
                           5.98214097e+197, 2.14033053e-308, 1.37426566e-048, 1.37359775e-048, 5.98214217e+197,
                           4.98207947e-307, 5.05214788e+173, 5.44132692e+174, 6.57819254e+198, 3.35173491e-309,
                           5.44132692e+174, 1.58913161e-234]]),
        'Rows': ['A1', 'B1', 'C1', 'D1', 'E1', 'F1', 'G1', 'H1', 'I1', 'J1', 'K1', 'L1', 'M1', 'N1', 'O1', 'P1', 'Q1',
                 'R1', 'S1', 'T1', 'U1', 'V1', 'W1', 'X1', 'Y1', 'Z1', 'A2', 'B2', 'C2', 'D2', 'E2', 'F2', 'G2', 'H2',
                 'I2', 'J2', 'K2', 'L2', 'M2', 'N2', 'O2', 'P2', 'Q2', 'R2', 'S2', 'T2', 'U2', 'V2', 'W2', 'X2', 'Y2',
                 'Z2', 'A3', 'B3', 'C3', 'D3', 'E3', 'F3', 'G3', 'H3', 'I3', 'J3', 'K3', 'L3', 'M3', 'N3', 'O3', 'P3',
                 'Q3', 'R3', 'S3', 'T3', 'U3', 'V3', 'W3', 'X3', 'Y3', 'Z3', 'A4', 'B4', 'C4', 'D4', 'E4', 'F4', 'G4',
                 'H4', 'I4', 'J4', 'K4', 'L4', 'M4', 'N4', 'O4', 'P4', 'Q4', 'R4', 'S4', 'T4', 'U4', 'V4', 'W4', 'X4',
                 'Y4', 'Z4', 'A5', 'B5', 'C5', 'D5', 'E5', 'F5', 'G5', 'H5', 'I5', 'J5', 'K5', 'L5', 'M5', 'N5', 'O5',
                 'P5', 'Q5', 'R5', 'S5', 'T5', 'U5', 'V5', 'W5', 'X5', 'Y5', 'Z5', 'A6', 'B6', 'C6', 'D6', 'E6', 'F6',
                 'G6', 'H6', 'I6', 'J6', 'K6', 'L6', 'M6', 'N6', 'O6', 'P6', 'Q6', 'R6', 'S6', 'T6', 'U6', 'V6', 'W6',
                 'X6', 'Y6', 'Z6', 'A7', 'B7', 'C7', 'D7', 'E7', 'F7', 'G7', 'H7', 'I7', 'J7', 'K7', 'L7', 'M7', 'N7',
                 'O7', 'P7', 'Q7', 'R7', 'S7', 'T7', 'U7', 'V7', 'W7', 'X7', 'Y7', 'Z7', 'A8', 'B8', 'C8', 'D8', 'E8',
                 'F8', 'G8', 'H8', 'I8', 'J8', 'K8', 'L8', 'M8', 'N8', 'O8', 'P8', 'Q8', 'R8', 'S8', 'T8', 'U8', 'V8',
                 'W8', 'X8', 'Y8', 'Z8', 'A9', 'B9', 'C9', 'D9', 'E9', 'F9', 'G9', 'H9', 'I9', 'J9', 'K9', 'L9', 'M9',
                 'N9', 'O9', 'P9', 'Q9', 'R9', 'S9', 'T9', 'U9', 'V9', 'W9', 'X9', 'Y9', 'Z9', 'A10', 'B10', 'C10',
                 'D10', 'E10', 'F10', 'G10', 'H10', 'I10', 'J10', 'K10', 'L10', 'M10', 'N10', 'O10', 'P10', 'Q10',
                 'R10', 'S10', 'T10', 'U10', 'V10', 'W10', 'X10', 'Y10', 'Z10', 'A11', 'B11', 'C11', 'D11', 'E11',
                 'F11', 'G11', 'H11', 'I11', 'J11', 'K11', 'L11', 'M11', 'N11', 'O11', 'P11', 'Q11', 'R11', 'S11',
                 'T11', 'U11', 'V11', 'W11', 'X11', 'Y11', 'Z11', 'A12', 'B12', 'C12', 'D12', 'E12', 'F12', 'G12',
                 'H12', 'I12', 'J12', 'K12', 'L12', 'M12', 'N12', 'O12', 'P12', 'Q12', 'R12', 'S12', 'T12', 'U12',
                 'V12', 'W12', 'X12', 'Y12', 'Z12', 'A13', 'B13', 'C13', 'D13', 'E13', 'F13', 'G13', 'H13', 'I13',
                 'J13', 'K13', 'L13', 'M13', 'N13', 'O13', 'P13', 'Q13', 'R13', 'S13', 'T13', 'U13', 'V13', 'W13',
                 'X13', 'Y13', 'Z13', 'A14', 'B14', 'C14', 'D14', 'E14', 'F14', 'G14', 'H14', 'I14', 'J14', 'K14',
                 'L14', 'M14', 'N14', 'O14', 'P14', 'Q14', 'R14', 'S14', 'T14', 'U14', 'V14', 'W14', 'X14', 'Y14',
                 'Z14', 'A15', 'B15', 'C15', 'D15', 'E15', 'F15', 'G15', 'H15', 'I15', 'J15', 'K15', 'L15', 'M15',
                 'N15', 'O15', 'P15', 'Q15', 'R15', 'S15', 'T15', 'U15', 'V15', 'W15', 'X15', 'Y15', 'Z15', 'A16',
                 'B16', 'C16', 'D16', 'E16', 'F16', 'G16', 'H16', 'I16', 'J16', 'K16', 'L16', 'M16', 'N16', 'O16',
                 'P16', 'Q16', 'R16', 'S16', 'T16', 'U16', 'V16', 'W16', 'X16', 'Y16', 'Z16', 'A17', 'B17', 'C17',
                 'D17', 'E17', 'F17', 'G17', 'H17', 'I17', 'J17', 'K17', 'L17', 'M17', 'N17', 'O17', 'P17', 'Q17',
                 'R17', 'S17', 'T17', 'U17', 'V17', 'W17', 'X17', 'Y17', 'Z17', 'A18', 'B18', 'C18', 'D18', 'E18',
                 'F18', 'G18', 'H18', 'I18', 'J18', 'K18', 'L18', 'M18', 'N18', 'O18', 'P18', 'Q18', 'R18', 'S18',
                 'T18', 'U18', 'V18', 'W18', 'X18', 'Y18', 'Z18', 'A19', 'B19', 'C19', 'D19', 'E19', 'F19', 'G19',
                 'H19', 'I19', 'J19', 'K19', 'L19', 'M19', 'N19', 'O19', 'P19', 'Q19', 'R19', 'S19', 'T19', 'U19',
                 'V19', 'W19', 'X19', 'Y19', 'Z19', 'A20', 'B20', 'C20', 'D20', 'E20', 'F20', 'G20', 'H20', 'I20',
                 'J20', 'K20', 'L20', 'M20', 'N20', 'O20', 'P20', 'Q20', 'R20', 'S20', 'T20', 'U20', 'V20', 'W20',
                 'X20', 'Y20', 'Z20', 'A21', 'B21', 'C21', 'D21', 'E21', 'F21', 'G21', 'H21', 'I21', 'J21', 'K21',
                 'L21', 'M21', 'N21', 'O21', 'P21', 'Q21', 'R21', 'S21', 'T21', 'U21', 'V21', 'W21', 'X21', 'Y21',
                 'Z21', 'A22', 'B22', 'C22', 'D22', 'E22', 'F22', 'G22', 'H22', 'I22', 'J22', 'K22', 'L22', 'M22',
                 'N22', 'O22', 'P22', 'Q22', 'R22', 'S22', 'T22', 'U22', 'V22', 'W22', 'X22', 'Y22', 'Z22', 'A23',
                 'B23', 'C23', 'D23', 'E23', 'F23', 'G23', 'H23', 'I23', 'J23', 'K23', 'L23', 'M23', 'N23', 'O23',
                 'P23', 'Q23', 'R23', 'S23', 'T23', 'U23', 'V23', 'W23', 'X23', 'Y23', 'Z23', 'A24', 'B24', 'C24',
                 'D24', 'E24', 'F24', 'G24', 'H24', 'I24', 'J24', 'K24', 'L24', 'M24', 'N24', 'O24', 'P24', 'Q24',
                 'R24', 'S24', 'T24', 'U24', 'V24', 'W24', 'X24', 'Y24', 'Z24', 'A25', 'B25', 'C25', 'D25', 'E25',
                 'F25', 'G25', 'H25', 'I25', 'J25', 'K25', 'L25', 'M25', 'N25', 'O25', 'P25', 'Q25', 'R25', 'S25',
                 'T25', 'U25', 'V25', 'W25', 'X25', 'Y25', 'Z25', 'A26', 'B26', 'C26', 'D26', 'E26', 'F26', 'G26',
                 'H26', 'I26', 'J26', 'K26', 'L26', 'M26', 'N26', 'O26', 'P26', 'Q26', 'R26', 'S26', 'T26', 'U26',
                 'V26', 'W26', 'X26', 'Y26', 'Z26'],
        'Header': ['f1s1t1c2test_Basic_Otsu_Threshold_Thresholded_Pixel_Area',
                   'f1s1t1c1test_Basic_Otsu_Threshold_Thresholded_Pixel_Area',
                   'f3s1t1c1test_Basic_Otsu_Threshold_Thresholded_Pixel_Area',
                   'f4s1t1c1test_Basic_Otsu_Threshold_Thresholded_Pixel_Area',
                   'f2s1t1c1test_Basic_Otsu_Threshold_Thresholded_Pixel_Area',
                   'f4s1t1c2test_Basic_Otsu_Threshold_Thresholded_Pixel_Area',
                   'f2s1t1c2test_Basic_Otsu_Threshold_Thresholded_Pixel_Area',
                   'f3s1t1c2test_Basic_Otsu_Threshold_Thresholded_Pixel_Area']}
        return r

    def addDataset(self):
        pass

    def dataPath(self):
        return self._dataPath;

    def loadableWellPlates(self):
        return self._loadableWellPlate

    def addPath(self, str):
        self._dataPath.append(str)

    def deletePath(self, str):
        self._dataPath.remove(str)

    def loadWellplate(self, str):
        if str in self._loadableWellPlate:
            t = str.split('/')
            #            print(t[len(t)-2])
            return self._loadedWellPlates.append(t[len(t) - 2])
        raise 'error trying to load unavailable data'

    def getSelectedImageFileNames(self):
        if self._selected:
            return self._selectedFileName

    def getLoadedImageFileNames(self):
        return self._loadedFileNames

    def selectWells(self, str, lstr):
        if str in self._loadedWellPlates:
            if 'B4' in lstr:
                self._selected = True
            else:
                self._selected = False
            return
        raise 'error selecting well'

    def displaySelection(self):
        # Do nothing is expected
        pass

    def showImageFile(self, listfile):
        # Do nothing is expected
        pass


if __name__ == '__main__':
    global checkout
    checkout = CheckoutFake()

# Ask the Gui to load first loadable wellplate
checkout.loadWellplate(checkout.loadableWellPlates()[0])
# checkout.get

checkout.selectWells(checkout.loadedWellPlates()[0], ['B4'])
# Display the selection
checkout.displaySelection()
# print(list(checkout.availableDatasets().keys())[0])
# print(list(checkout.availableDatasets().values())[0][0])
#dataset = checkout.getDataset([list(checkout.availableDatasets().keys())[0]],
#                              [list(checkout.availableDatasets().values())[0][0]])

#print(np.shape(dataset['Data']))
#print(np.shape(dataset['Data'][0]))
#print(np.shape(dataset['Data'][1]))
#print(np.shape(dataset['Data'][2]))
#print(np.shape(dataset['Data'][3]))
#print(np.shape(dataset['Data'][4]))
#print(np.shape(dataset['Data'][5]))
#print(np.shape(dataset['Data'][6]))
#print(np.shape(dataset['Data'][7]))
#print(dataset['Data'][:][np.nonzero(dataset['activeRows'])])
