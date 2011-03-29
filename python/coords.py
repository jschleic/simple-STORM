#!/usr/bin/env python

import csv
import sys
import numpy as np
import scipy.stats

def readfile(filename):
	''' read file with coordinates of found spots and 
	convert it into a numpy-array'''
	
	csvfile = open(filename, "rb")
	dialect = csv.Sniffer().sniff(csvfile.read(1024))
	csvfile.seek(0)
	reader = csv.reader(csvfile, dialect)
	
	# read dimensions
	dimline = reader.next()
	dimline = [float(i) for i in dimline] #convert to float
	coords = []
	try:
		for row in reader:
			coords.append(row)
	except csv.Error, e:
		sys.exit('file %s, line %d: %s' % (filename, reader.line_num, e))
		
	npcoords = np.array(coords, dtype='float') #numpy-array
	return (dimline[0],dimline[1]), npcoords


def writefile(filename, shape, coords):
	'''write an array of coordinates found in an image of dimensions 
	shape back to disk.'''
	
	fp = open(filename, 'wb')
	csvwriter = csv.writer(fp, delimiter=' ')
	shape2 = [int(j) for j in shape] # map tuple to array of ints
	shape2.append(int(np.max(coords[:,2]))+1) # number of frames
	csvwriter.writerow(shape2)
	csvwriter.writerows(coords)
	fp.close()


def cropROI(coords, roi):
	'''select only spots inside the roi=[xmin, xmax, ymin, ymax]'''
	
	xmin, xmax, ymin, ymax = roi
	idx1 = coords[:,0] >= xmin
	idx2 = coords[:,0] <  xmax
	idx3 = coords[:,1] >= ymin
	idx4 = coords[:,1] <  ymax
	
	idxs = np.all([idx1,idx2,idx3,idx4], axis=0)
	
	return coords[idxs,:]

def coords2Image(dimension, coords, factor=8):
	im = np.zeros((dimension[0]*factor, dimension[1]*factor))
	for c in coords:
		x,y, intensity = c[0], c[1], c[3]
		im[y*factor,x*factor] += intensity
	#limit maximum value
	mmx = scipy.stats.scoreatpercentile(im.flat, 99.6)
	im[im>mmx] = mmx
	return im
