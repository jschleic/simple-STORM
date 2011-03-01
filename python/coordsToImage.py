import matplotlib.pyplot as plt
import numpy as np
import sys

def coordsShow(c, w, h, factor=8, maxpercentile=0.996):
	''' Generate resulting image from a list of 
	coordinates c.
	'''
	import vigra as v
	im = v.ScalarImage([w*factor, h*factor])
	for x, y, slice, intensity, z in c:
		im[x*factor,y*factor] += intensity
	# TODO: limit to maxpercentile as in C++ code
	return im

def plot_coords(c, format='r,'):
	''' Scatterplot over all found coordinates'''
	cc = np.array(c)
	plt.plot(cc[:,0], cc[:,1], format)

def readCoordsFile(filename):
	coords = []
	myfile = open(filename, 'r')
	xres, yres, stacksize = map(int, myfile.readline().split(" ")) #first elem gives image dimensions
	
	for line in myfile:
		tmp = line.split(" ")
		try:
			for j in range(len(tmp)):
				tmp[j] = float(tmp[j])
		except ValueError:
			print "Value Error in line " + line
			break												
		coords.append(tmp)
	myfile.close()
	return coords, xres, yres

def saveCoordsRoi(coords, roi, filename):
	f = open(filename, 'w')
	xmin, xmax, ymin, ymax = roi
	xlist = []
	ylist = []
	for x, y, slice, intensity, z in coords:
		if x >= xmin and x < xmax and y >= ymin and y < ymax:
			xlist.append(x)
			ylist.append(y)
	for i in range(len(xlist)):
		f.write(str(xlist[i]) + ",\t" + str(ylist[i]) + "\n")

if __name__ == "__main__":
	'''read image coordinates, construct images
	save output as .png or .tiff ...'''
	if len(sys.argv) < 3:
		print ( "Usage: " + sys.argv[0] + "coordsfile.txt image.png")
		sys.exit(1)
	coords, w, h = readCoordsFile(sys.argv[1])
	im = coordsShow(coords, w, h)
	import vigra as v
	v.impex.writeImage(im, sys.argv[2])
