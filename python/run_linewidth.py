#!/usr/bin/env python
import sys
import numpy as np
import robustRegression as robreg
import scipy.stats

import coords as cr


class AffineLinearModel:
    """linear system

    This class is used as a helper to determine the residuals
    """
    def __init__(self):
        self.input_columns = [0,1] # two input columns
        self.output_columns = [2]  # one output column
    def get_error( self, data, model):
        A = np.vstack([data[:,i] for i in self.input_columns]).T
        B = np.vstack([data[:,i] for i in self.output_columns]).T
        B_fit = np.dot(A,model)
        err_per_point = np.sum((B-B_fit)**2,axis=1) # sum squared error per row
        return err_per_point
 
 
def remove_outliers(d, fit, threshold = 0.1):
    ws = np.asarray(fit["Weights"])
    ws[ws < threshold] = 0
    legalIndices = np.nonzero(ws)
    legalPairs = np.array(d)[legalIndices]
    return legalPairs

def robust_measure(coords):
	xs = coords[:,0]
	ys = coords[:,1]
	ll = len(coords)
	fit = robreg.lmrob(xs,ys)
	all_data = np.hstack( [xs.reshape([ll,1]),np.ones((ll,1)),ys.reshape([ll,1])] )
	inliers = remove_outliers(all_data, fit)
	bestfit = np.array([[fit['a']],[fit['b']]])
	
	model = AffineLinearModel()
	resids = model.get_error(inliers, bestfit)
	estimLWPercentile = scipy.stats.scoreatpercentile(resids, 95)
	return estimLWPercentile, inliers, bestfit

if __name__ == "__main__":
	if not len(sys.argv) == 6:
		print ( "Usage: " + sys.argv[0] + "coordsfile.txt xmin xmax ymin ymax")
		sys.exit(1)
	filename = sys.argv[1]
	roi = [int(sys.argv[j]) for j in range(2,6)]
	dimensions, coords = cr.readfile(filename)
	coords = cr.cropROI(coords, roi)
	lw, inl, fit = robust_measure(coords)
	print filename, np.sqrt(lw)
	
