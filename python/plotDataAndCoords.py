#!/usr/bin/python

import matplotlib.pyplot as plt
import numpy as np
import h5Wrapper as h5
import coordsToImage as coord2im

import sys


def pltImAndCoords(h5file, frame, coordFiles, printerFriendly=True):
	'''read image coordinates, plot them together with the raw data
	save output as .png or .tiff or display with matplotlib'''
	
	# read input data
	rawData = h5.readHDF5Frame(h5file, frame)
	
	coordDat = [(coord2im.readCoordsFile(f)) for f in coordFiles]
	
	if printerFriendly:
		rawData = -rawData
	plt.imshow(rawData, interpolation='nearest')
	plt.colorbar()
	plt.gray()
	axx = plt.axis()
	markers = ['r+', 'bx', 'go', 'k,', 'co', 'yo']
	
	for i in range(len(coordFiles)):
		c, w, h = coordDat[i]
		cc = np.array(c)
		idxs = (cc[:,2] == frame)
		cc = cc[idxs]
		ll = coordFiles[i][coordFiles[i].rfind('/')+1:]
		plt.plot(cc[:,0], cc[:,1], markers[i], label=ll)
	plt.legend() 
	plt.axis(axx) # dont change axis by plotting coordinates
	plt.show()

if __name__ == "__main__":
	if not 3 <= len(sys.argv) < 6:
		print ( "Usage: " + sys.argv[0] + " data.h5 frameNr coords1.txt [coords2.txt [coords3.txt]]")
		sys.exit(1)
	coordFiles = [sys.argv[i] for i in range(3,len(sys.argv))]
	frame = int(sys.argv[2])
	
	pltImAndCoords(sys.argv[1], frame, coordFiles)
	
