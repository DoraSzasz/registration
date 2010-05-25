#!/usr/bin/env python

# external packages
import sys
from IPython.Shell import IPShellEmbed
ipshell = IPShellEmbed()
# ipshell()

# my packages
from sources.versor_reader import VersorReader
from sequences.matplotlib_plotter import MatplotlibPlotter
from sequences.mayavi_plotter import MayaviPlotter

# extract command line arguments
output_dir = sys.argv[1]

# parse input file
params3D = VersorReader(output_dir + "/output3D.txt")

# plotter = MatplotlibPlotter(output_dir + "/movies", singleres)
plotter = MayaviPlotter(output_dir + "/movies", params3d)
plotter.plot()
