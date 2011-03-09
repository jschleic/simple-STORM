import numpy as np
import matplotlib.pyplot as plt

import coordsToImage as coord2im


#config
#~ filename = 'Bead1_647_BS1_77575_568LP.txt'
filename = 'Bead1_532_BS1_58060_532LP.txt'
#~ filename = 'Bead1_647_BS1_77575_568LP_thresh600.txt'
maxDist = 8


def dist(a, b):
	a1 = np.array(a)
	b1 = np.array(b)
	return np.linalg.norm(a1[:2]-b1[:2])

def nearestNeighbor(p, beads, maxDist):
	x,y,intensity = p
	nearestIdx = 0 #initial guess
	nearestDist = 32768 #large number
	for i in range(len(beads)):
		if not x-maxDist <= beads[i][0][0] <= x+maxDist:
			continue
		if not y-maxDist <= beads[i][0][1] <= y+maxDist:
			continue
		#check nearest neighbor
		if dist(beads[i][0], p)<nearestDist:
			nearestIdx = i
			nearestDist = dist(beads[i][0], p)
	return nearestDist, nearestIdx

cc, a, b = coord2im.readCoordsFile(filename)

beads = []
ign=0
# sort beads in 'bins'
for x, y, frame, intensity, z in cc:
	if frame==0: #define bins
		nearestDist, nearestIdx = nearestNeighbor((x,y,intensity), beads, maxDist)
		if nearestDist > maxDist:
			beads.append([(x,y,intensity)])
		else:
			print "removing beads too close together: ", (x,y,intensity), beads[nearestIdx]
			beads.remove(beads[nearestIdx]) 
	else:
		nearestDist, nearestIdx = nearestNeighbor((x,y,intensity), beads, maxDist)
		if nearestDist>maxDist:  # no bin found
			#~ print "ignoring.", (x,y,intensity)
			ign += 1
			continue #ignore this bead
		else: #push
			#~ print "adding.", (x,y,intensity)
			beads[nearestIdx].append( (x,y,intensity) )

print "num beads: ", len(beads)
print ign, "single detections ignored."
skipped = 0 #counter
scatterplotData = []
meanData = []

for candidate in beads:
	if len(candidate) > 1000 or len(candidate) < 990: # allowing 1% not detected
		skipped += 1
		continue
	positions, intensities = np.hsplit(np.array(candidate), np.array([2]))
	mm = np.mean(positions, 0)
	dists = np.array([np.linalg.norm(positions[i]-mm) for  i in range(len(positions))])
	#
	stddev = np.sqrt(np.mean(dists**2))
	intensity = np.mean(intensities)
	print "variance of dist: ", stddev, "@intensity: ", intensity
	scatterplotData.append( (intensity, stddev))
	meanData.append(mm)
	
print skipped, "beads skipped."

#plot:
xx,yy=np.hsplit(np.array(scatterplotData),np.array([1]))
plt.plot(xx,yy,'rx')
plt.xlabel('Beat Intensity')
plt.ylabel('stddev of detected position')
plt.title('STORM: localisation precision at different beat intensities')
plt.ylabel('stddev of detected position')

plt.figure()
xx,yy=np.hsplit(np.array(meanData),np.array([1]))
plt.plot(xx,yy,'ro')
plt.title(filename)
plt.xlabel('x')
plt.ylabel('y')
