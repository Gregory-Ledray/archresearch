#this script performs several tasks with inputs:
#1. an initial stream of instructions
#2. Stopping criteria for the genetic algorithm

#procedure
#first, it configures the architectural simulator
#second, it identifies an input file of instructions for gem5 and runs the simulator on them
#third, it takes the outputs from gem5 and runs McPAT on them
#fourth, it takes the outputs from the simulator and McPAT and runs VoltSpot on them
#fifth, the outputs from the simulator, McPAT and VoltSpot are converted to run
#as inputs in a data mining package, GAUL, and configures the stopping criteria
#sixth, run the data mining package until a stopping criterion is met
#seventh, take the output from the data mining package and convert it into a file of
#instructions for gem5
import os, sys
#conditional imports
if os.name == 'posix' and sys.version_info[0] < 3:
    import subprocess32 as subprocess
else:
    import subprocess

#functions for running programs
def build_gem5():
    subprocess.check_call('cd /home/grego/gem5/gem5', shell=True)
    subprocess.check_call('scons build/X86/gem5.opt', shell=True)
    os.chdir(os.path.expanduser('~/gem5/gem5'))
def run_gem5():
    subprocess.check_call('cd /home/grego/gem5/gem5', shell=True)
    subprocess.check_call('build/X86/gem5.opt configs/example/se.py -c tests/test-progs/hello/bin/x86/linux/hello', shell=True)
    os.chdir(os.path.expanduser('~/gem5/gem5'))

#main
os.chdir(os.path.expanduser('~/gem5/gem5'))
try:
    run_gem5()
except subprocess.CalledProcessError as (retcode, cmd):
    if retcode == 127:
        build_gem5()
        run_gem5()
