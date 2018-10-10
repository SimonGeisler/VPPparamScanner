Author: Simon Geisler
Date:10.10.2018

This is a tool to conveniently scan parameters using SPheno[https://spheno.hepforge.org/] and VevaciousPlusPlus [https://github.com/benoleary/VevaciousPlusPlus] alias V++.

This is a free opensource software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation (http://www.gnu.org/licenses/)

Installation:
-------------

Preliminaries

1. Install VevaciousPlusPlus (Instructions given in the README in the V++ github)

2. Get Fortran SPheno code for your model. If you don't have it, get the Mathematica package SARAH  [https://sarah.hepforge.org/] and follow the instructions to generate SPheno code

3. Get VevaciousPlusPlus model files (MyModel.vin and ScaleAndblock.xml file) for your chosen model. If you don't have one load the Model you want in SARAH and generate a model file using "MakeVevacious[Version->"++"]"

Installation of VPPparamScanner

1. Download the exprtk header files [https://github.com/ArashPartow/exprtk] The exprtk.hpp file should suffice. Also give the correct path to the exprtk.hpp header in the makefile.

2. Get the boost libraries. Not the newest ones are needed, the libboost-dev package from your ubuntu should suffice

3. Your g++ compiler should at least allow C++11 standard, but normally this should be fine

4. Run "make" in the main folder which contains the "Makefile"

Setting up the configs and Initializationfiles

To do this, just copy the template which is present in the "Modelfiles" directory and change it the way you need it.

1. Put the LesHouches.in, MyModel.vin and ScaleAndBlock file in your model folder

2. Change your config file so that the paths match with your configuration 

3. Change the Initializationfiles so that the paths match and change the corresponding ObjectInitialization files to your needs. You need to for example change the paths to the MyModel.vin and ScaleAndBlock.xml file to your files in PotentialfunctionInitialization.xml

4. Now, after you did the most important steps, you can change the config file as you want. An example config file is given in templates - with the most important instructions given.

5. Run ./VPPparamScanner ./directory/to/your/configfile.xml

6. Win!
