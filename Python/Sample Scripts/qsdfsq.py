# Ask the Gui to load first loadable wellplate

import json
from skimage import io
import numpy as np

if checkout.state == "setup":
    checkout.description('Demo Python script', ['wiest.daessle@gmail.com'], 'Test script for python specs!!!')
    checkout.use('refImage', checkout.Image2D , description='boby' , vector_image=True)
 #   checkout.use_enum('Algorithm', ['Linear', 'Cubic', 'OrthoCubic'], default='OrthoCubic')
    #checkout.use_enum('Algorithm2', ['TriLinear', 'SubCubic', 'OrthoCubic'], default='OrthoCubic')
#    checkout.produce('foo', checkout.Int, 'the foo of the bar value')
    checkout.produce('result', checkout.Image2D, 'Result Image')
    print("Setting up");


if checkout.state == "run":
    print("Starting Script")
    checkout.foo = 10
    checkout.refImage_dec = json.loads(checkout.refImage)
    checkout.refImageC0=io.imread(checkout.refImage_dec['Data'][0])
    checkout.refImageC1=io.imread(checkout.refImage_dec['Data'][1])


    checkout.result=np.zeros(shape=(100,100, 3), dtype=np.uint16)


#    print(checkout.refImage)
    print("Finished Script")


# checkout.Image2D
# checkout.TimeImage
# checkout.StackedImage
# checkout.TimeStackedImage
# checkout.ImageXP
# checkout.TimeImageXP
# checkout.StackedImageXP
# checkout.TimeStackedImageXP
