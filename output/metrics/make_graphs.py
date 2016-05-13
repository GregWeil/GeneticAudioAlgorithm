import glob
import matplotlib.pyplot as plt
import numpy as np

class point():
    def __init__(self):
    	self.soundfilename = ""
        self.ranks = 0
        self.threads_per_rank = 0
        self.pop_per_rank = 0
        self.pop_total = 0
        self.generations = 0
        self.time = 0.0
        self.fitness = 0.0


datasets = []
for filename in glob.glob('*.txt'):
	p = point()
	with open(filename, 'r') as f:
		for i, line in enumerate(f):
			if(i == 0):
				p.soundfilename = line.partition(" ")[0]
			if(i == 1):
				p.ranks = int(line.partition(" ")[0])
			if(i == 2):
				p.threads_per_rank = int(line.partition(" ")[0])
			if(i == 3):
				p.pop_per_rank = int(line.partition(" ")[0])
			if(i == 4):
				p.generations = int(line.partition(" ")[0])
			if(i == 5):
				p.time = float(line.partition(" ")[0])
			if(i == 6):
				p.fitness = float(line.partition(" ")[0])
				break;
	p.pop_total = p.ranks*p.pop_per_rank
	added = False
	for dataset in datasets:
		if dataset[0].threads_per_rank == p.threads_per_rank:
			added = True;
			dataset.append(p)
	if not added:
		newset = [p]
		datasets.append(newset)

print("done adding. have " + str(len(datasets)) + " sets")
for dataset in datasets :
	print("pop " + str(dataset[0].pop_total) + ". size " + str(len(dataset)))


colors = ['r','y','b','g','k','m','c','w']
for i, dataset in enumerate(datasets) :
	#different colors for different pop_total
	ranks = []
	time = []
	fitness = []
	for p in dataset:
		ranks.append(p.ranks)
		time.append(p.time)
		fitness.append(p.fitness)
	ranks = np.asarray(ranks)
	time = np.asarray(time)
	fitness = np.asarray(fitness)

	print(p.threads_per_rank)

	#sort arrays by number of ranks(so lines draw correctly)
	ranks, time, fitness = (list(t) for t in zip(*sorted(zip(ranks, time, fitness))))

	#graph1: ranks on x, time on y.
	#graph2: ranks on x, fitness on y.

	plt.figure(1)
	plt.plot(ranks, time, colors[i])

	plt.figure(2)
	plt.plot(ranks, fitness, colors[i])

	print("total pop size " + str(dataset[0].pop_total) + " has color " + colors[i])

plt.show()