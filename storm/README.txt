/************************************************************************/
/*                                                                      */
/*                  ANALYSIS OF STORM DATA                              */
/*                                                                      */
/*    Copyright 2010-2011 by Joachim Schleicher and Ullrich Koethe      */
/*                                                                      */
/*    Please direct questions, bug reports, and contributions to        */
/*    joachim.schleicher@iwr.uni-heidelberg.de                          */
/************************************************************************/


This software takes an Anodor sif Image Stack as input and produces a
super-resolution image as output.
Supported output formats are bmp (native) and png, tiff, jpeg using 
libpng, libtiff, libjpeg respectively.

Parameters are the enlargement factor and the background noise threshold.

To run the program with default parameters: storm.exe input.sif
To display a short help: storm.exe --help

As a test you can run the algorithm on the TestImage:
storm testSif_4_16_30001.sif out.png --coordsfile=outCoords.txt
diff out.png testReference.png
diff outCoords.txt testCoords.txt
