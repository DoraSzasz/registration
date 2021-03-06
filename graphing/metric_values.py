from os.path import *
from os import listdir
from numpy import genfromtxt, array

class MetricValues:
    def __init__(self, transform_dir):
        self._values = [genfromtxt(join(transform_dir, file)) for file in listdir(transform_dir)]
    
    def values(self):
        return self._values
    
    def delta_values(self):
        # subtract the previous value from each value
        return [row[1:] - row[:-1] for row in self._values]
    
    def initial_values(self):
        return [row[0] for row in self._values]
        
    def final_values(self):
        return [row[-1] for row in self._values]
    
    def number_of_slices(self):
        return len(self._values)
        