#!/usr/bin/env python

import csv
import sys
import numpy as np
import scipy.stats
#import matplotlib.pyplot as plt

def readfile(filename):
	''' read file with coordinates of found spots and 
	convert it into a numpy-array'''
	
	try:	# try to load cached numpy file format (faster)
		npzfilename = str(filename)
		if npzfilename[-4:] != '.npz':
			npzfilename += '.npz'
		data = np.load(npzfilename)
		dims = data['dims']
		coords = data['coords']
		print "I read the cached file instead of parsing a .txt file"
		return dims, coords
	except IOError: # file could not be read
		pass

	# read csv coordinates list
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
		
	npcoords = np.asarray(coords, dtype='float') #numpy-array
	npdims = np.asarray(dimline[0:3])
	np.savez(str(filename), dims=npdims, coords=npcoords)
	return npdims, npcoords


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
	idxs = (coords[:,0] >= xmin) & (coords[:,0] <  xmax) & (coords[:,1] >= ymin) & (coords[:,1] <  ymax)

	return coords[idxs,:]

def coords2Image(dimension, coords, factor=8):
	im = np.zeros((dimension[1]*factor, dimension[0]*factor))
	cc = np.array(coords[:,:2]*factor, dtype=int) # integer indices
	for i in xrange(len(coords)):
		intensity = coords[i,3]
		im[cc[i,1],cc[i,0]] += intensity
	#limit maximum value
	mmx = scipy.stats.scoreatpercentile(im.flat, 99.6)
	if mmx > 0:
		im[im>mmx] = mmx # crop maximum at above percentile
	return im

def intensity2Sigma(intensity):
	return 10./float(intensity) + 0.05

def gauss(intensity, sigma):
	c0 = 1/(2.*np.pi*sigma)*1.
	c1 = 1/(2.*np.pi*sigma)*np.exp(-1./(2.*sigma**2))
	c2 = 1/(2.*np.pi*sigma)*np.exp(-4./(2.*sigma**2))
	cs2 = 1/(2.*np.pi*sigma)*np.exp(-2./(2.*sigma**2))
	cs5 = 1/(2.*np.pi*sigma)*np.exp(-5./(2.*sigma**2))
	cs8 = 1/(2.*np.pi*sigma)*np.exp(-8./(2.*sigma**2))
	return np.array([\
		[cs8, cs5, c2, cs5, cs8], \
		[cs5, cs2, c1, cs2, cs5], \
		[c2, c1, c0, c1, c2], \
		[cs5, cs2, c1, cs2, cs5],\
		[cs8, cs5, c2, cs5, cs8]\
		])*intensity

def coords2ImageSmooth(dimension, coords, factor=8):
	im = np.zeros((dimension[1]*factor, dimension[0]*factor))
	cc = np.array(coords[:,:2]*factor, dtype=int) # integer indices
	for i in xrange(len(coords)):
		intensity = coords[i,3]
		sigma = intensity2Sigma(intensity)
		im[cc[i,1]-2:cc[i,1]+3,cc[i,0]-2:cc[i,0]+3] += gauss(intensity, sigma*factor)
	#limit maximum value
	mmx = scipy.stats.scoreatpercentile(im.flat, 99.6)
	if mmx > 0:
		im[im>mmx] = mmx # crop maximum at above percentile
	return im

if __name__ == "__main__":
	'''read image coordinates, construct images
	save output as .png or .tiff ...'''
	if len(sys.argv) < 3:
		print ( "Usage: " + sys.argv[0] + " coordsfile.txt image.png")
		sys.exit(1)
	import matplotlib.pyplot as plt
	dims, coords = readfile(sys.argv[1])
	im = coords2Image(dims, coords, factor=8)
	plt.gray()
	plt.imsave(sys.argv[2], im)
