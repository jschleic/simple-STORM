import run_linewidth as lw
import coords as cr
import matplotlib.pyplot as plt
import numpy as np

import sys
def plt_overview(dimension, coords, roi):
	# roi rectangle
	mybox=np.array([[roi[0],roi[0], roi[1], roi[1], roi[0]],[roi[2],roi[3],roi[3],roi[2],roi[2]]])

	#construct image from coordinates
	zoom_factor = 8
	im = cr.coords2Image(dimension, coords, zoom_factor)

	#plot
	plt.figure()
	plt.title("Storm image: " + overviewfile)
	plt.imshow(im)
	plt.hot() #colormap

	# rescale roi rectangle and plot it into the image
	mybox=zoom_factor*mybox
	plt.plot(mybox[0,...], mybox[1,...], '0.3')
	plt.axis([0,dimension[0]*zoom_factor,0,dimension[1]*zoom_factor])
	plt.colorbar()

# Plot a Line y = m*x + b. Fit = [m,b]
def plotLine(fit, xmin, xmax):
	plt.plot(np.arange(xmin+1, xmax), 
				np.dot(np.arange(xmin+1, xmax),fit[0,0])+fit[1,0],
				label='robust fit' )

def plt_fit(coords, inliers, roi, fit):
	plt.figure()
	plt.plot(coords[:,0], coords[:,1], 'b,')
	plt.plot(inliers[:,0], inliers[:,2], 'rx')
	plotLine(fit, roi[0], roi[1])
	
	
if __name__ == "__main__":
	path = r'./'
	files = [ # list of files to compare
			'20a_MT_Atto520-coords-HCI.txt']
	roi = [77,91,72,82]
	#~ roi = [91,105,14,33 ]

	# Overview plot (whole image)
	overviewfile=files[0]
	dims, coords = cr.readfile(path+overviewfile)
	plt_overview(dims, coords, roi)

	for f in files:
		print "processing file " + f
		dims, coords = cr.readfile(path+f)
		coords = cr.cropROI(coords, roi)
		linewidth, inliers, fitparams = lw.robust_measure(coords)
		print np.sqrt(linewidth)

		#Plot data
		plt_fit(coords, inliers, roi, fitparams)
		plt.title("Data: " + f)
		plt.axis(roi)
		
		#plot residuals histogram
		model = lw.AffineLinearModel()
		resids = model.get_error(inliers, fitparams)		
		plt.figure()
		plt.title("Residuals: " + f)
		plt.hist(resids,bins=50,range=[0,2])

		plt.show()
