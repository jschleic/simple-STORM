import csv
import sys
import numpy as np
import robust_regression as robreg
import matplotlib.pyplot as plt
import vigra as v
import scipy.stats
import scipy

import coordsToImage


class LinearLeastSquaresModel:
    """linear system solved using linear least squares

    This class serves as an example that fulfills the model interface
    needed by the ransac() function.
    
    """
    def __init__(self,input_columns,output_columns,debug=False):
        self.input_columns = input_columns
        self.output_columns = output_columns
        self.debug = debug
    def fit(self, data):
        A = np.vstack([data[:,i] for i in self.input_columns]).T
        B = np.vstack([data[:,i] for i in self.output_columns]).T
        x,resids,rank,s = scipy.linalg.lstsq(A,B)
        return x
    def get_error( self, data, model):
        A = np.vstack([data[:,i] for i in self.input_columns]).T
        B = np.vstack([data[:,i] for i in self.output_columns]).T
        B_fit = scipy.dot(A,model)
        err_per_point = np.sum((B-B_fit)**2,axis=1) # sum squared error per row
        return err_per_point
 
# Plot a Line y = m*x + b. Fit = [m,b]
def plotLine(fit, xmin, xmax):
	plt.plot(np.arange(xmin+1, xmax), 
				np.dot(np.arange(xmin+1, xmax),fit[0,0])+fit[1,0],
				label='RANSAC fit' )

 
def remove_outliers(d, fit, threshold = 0.1):
    ws = np.asarray(fit["Weights"])
    ws[ws < threshold] = 0
    legalIndices = np.nonzero(ws)
    legalPairs = np.array(d)[legalIndices]
    return legalPairs

def robust_measure(coords, roi):
	xlist = []
	ylist = []
	xs = []
	ys = []
	xmin, xmax, ymin, ymax = roi
	for x, y, slice, intensity, z in coords:
		if x >= xmin and x < xmax and y >= ymin and y < ymax:
			xlist.append([x])
			ylist.append([y])
			xs.append(x)
			ys.append(y)
	fit = robreg.lmrob(xs,ys)
	all_data = np.hstack( [xlist,np.ones((len(xlist),1)),ylist] )
	n_inputs=2
	input_columns = range(n_inputs) # the first columns of the array
	output_columns = [n_inputs+0] # the last columns of the array
	model = LinearLeastSquaresModel(input_columns,output_columns,debug=False)

	inliers = remove_outliers(all_data, fit)
	bestfit = np.array([[fit['a']],[fit['b']]])
	plotLine(bestfit, xmin, xmax)
	plt.plot( inliers[:,0], inliers[:,2], 'bx', label='inliers data' )
	
	resids = model.get_error(inliers, bestfit)
	estimLWPercentile = scipy.stats.scoreatpercentile(resids, 95)
	return estimLWPercentile, resids
	
#################################################
plt.clf()

path = r'/net/gorgonzola/storage/jschleic/Documents/log/vigra-storm-cpp/'
files = [#'20a_MT_Atto520-coords.txt', 
		'20a_MT_Atto520-coords-nofilter-thr800.txt', 
		'20a_MT_Atto520-coords-wiener-thr800.txt', 
		'20a_MT_Atto520-coords-invansc-thr800.txt', 
		'20a_MT_Atto520-coords-luisier_analyze75-thr800.txt',
		'20a_MT_Atto520-coords-wiener-thr800-CatmullRom.txt',
		'20a_MT_Atto520-coords-rapidstorm.txt',
		'tmp.txt',
		'20a_MT_Atto520-coords-wiener-thr800-CMR-Ortsraumfilter.txt']

# Overview plot (whole image)
plt.figure(1)
overviewfile=files[1]
coords = coordsToImage.readCoordsFile(path+overviewfile)
plt.title("Storm image: " + overviewfile)
roi = [77,91,72,82]
#~ 89,105,44,62
#~ roi = [91,105,14,33 ]

# roi rectangle
mybox=np.array([[roi[0],roi[0], roi[1], roi[1], roi[0]],[roi[2],roi[3],roi[3],roi[2],roi[2]]])

if 0:
	coordsToImage.plot_coords(coords)
	# plot roi as rectangle
	plt.plot(mybox[0,...], mybox[1,...], '0.3')

else:
	scaling_factor = 8
	im = coordsToImage.coordsShow(coords, 128,128,scaling_factor)
	v.imshow(im)

	# rescale roi rectangle
	mybox=scaling_factor*mybox
	# plot roi as rectangle
	plt.plot(mybox[0,...], mybox[1,...], '0.3')
	plt.axis([0,1024,0,1024])
	plt.clim(0,4000)
	plt.colorbar()



myrobreg_measure = []
myrobreg_resids = []
for f in files:
	print "processing file " + f
	plt.figure()
	plt.title("Data: " + f)
	coords = coordsToImage.readCoordsFile(path+f)
	coordsToImage.plot_coords(coords)
	plt.axis(roi)
	#~ coordsToImage.saveCoordsRoi(coords, roi, f+"_roi1.txt")
	lw, resids = robust_measure(coords, roi)
	print np.sqrt(lw)
	myrobreg_measure.append( np.sqrt(lw) )
	myrobreg_resids.append( resids )
	plt.axis(roi)
	
	#plot residuals histogram
	plt.figure()
	plt.title("Residuals: " + f)
	plt.hist(resids,bins=50,range=[0,2])


#bar chart
N = len(files)
ind = np.arange(N)    # the x locations for the groups
width = 0.35       # the width of the bars: can also be len(x) sequence

plt.figure()
p1 = plt.bar(ind, myrobreg_measure,   width, color='r')

plt.ylabel('line width')
plt.title('STORM data analysis - quality measure evaluating filter methods')
plt.xticks(ind+width/2., ('no filter', 'wiener filter', 'invansc', 'luisier', 'wiener+CatmullRom', 'RapidStorm') )
#~ plt.yticks(np.arange(0,81,10))

#boxplot
plt.figure()
plt.boxplot(myrobreg_resids, positions=ind, widths=width)
plt.ylabel('residuals')
plt.title('STORM data analysis - quality measure evaluating filter methods')
plt.xticks(ind, ('no filter', 'wiener filter', 'invansc', 'luisier', 'wiener+CatmullRom', 'RapidStorm') )

plt.show()
