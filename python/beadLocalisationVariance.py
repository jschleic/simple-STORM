import numpy as np

#ipython history
#02.03.2011

run -n /export/home/jschleic/devel/vigra_storm/src/private/python/plotDataAndCoords.py

pltImAndCoords('Bead1_532_BS1_58060_532LP.sif.h5', 0, ['Bead1_532_BS1_58060_532LP.txt','Bead1_647_BS1_77575_568LP.txt'], True)

rot, a, b = readCoordsFile('Bead1_647_BS1_77575_568LP.txt')
roi=[315, 365, 95, 140]
xmin, xmax, ymin, ymax = roi
for x, y, slice, intensity, z in rot:
	if x >= xmin and x < xmax and y >= ymin and y < ymax:
		rot_xlist.append(x)
		rot_ylist.append(y)
  
len(rot_xlist)
len(rot_ylist)
rot_x = np.array(rot_xlist)
rot_y = np.array(rot_ylist)

xx = np.array([rot_x])
yy = np.array([rot_y])
asdf = np.hstack([xx.transpose(), yy.transpose()])
np.mean(asdf, 0)
np.var(asdf, 0)

mm = np.mean(asdf, 0)
dists = [np.sqrt(np.dot((asdf[i]-mm), (asdf[i]-mm))) for  i in range(len(asdf))]

np.mean(dists)
np.std(dists)
