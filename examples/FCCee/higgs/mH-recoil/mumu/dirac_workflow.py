######################################################################################
### This script steers the execution of FCCAnalysis stage 1 in DIRAC (iLCDirac) 
######################################################################################

# Create sandbox files
import os
from shutil import copy2
import array

# Utility function
def copywithreplace(filein, fileout, repls):
    # If no replacements, just copy the file
    if len(repls) == 0:
        copy2(filein, fileout)
        return
    # input file
    fin = open(filein, "rt")
    # output file to write the result to
    fout = open(fileout, "wt")
    # for each line in the input file
    for line in fin:
        # Apply each requested replacement
        ltmp = line
        for r in repls:
            lout = ltmp.replace(str(r[0]), str(r[1]))
            ltmp = lout
        fout.write(lout)
    # close input and output files
    fin.close()
    fout.close()

# DIRAC part
from DIRAC import S_OK, S_ERROR
from DIRAC.Core.Base import Script

# Define a simple class to hold the script parameters
class Params(object):
  def __init__(self):
    self.wms = 'wms'
  def setWMS(self, value):
    self.wms = value
    return S_OK()

# Instantiate the params class
cliParams = Params()
Script.registerSwitch('w', 'wms', "WMS where to run", cliParams.setWMS)
Script.parseCommandLine(ignoreErrors=False)
# Get the list of services (the switch above appearer as servicesList[0])
servicesList = Script.getPositionalArgs()
print(servicesList)

from ILCDIRAC.Interfaces.API.DiracILC import DiracILC
from ILCDIRAC.Interfaces.API.NewInterface.UserJob import UserJob
from ILCDIRAC.Interfaces.API.NewInterface.Applications import GenericApplication

dIlc = DiracILC()

job = UserJob()
job.setOutputSandbox(['*.log', '*.sh', '*.py', '*.xml'])
outputdatafile='p8_ee_ZZ_ecm240.root'
job.setOutputData(outputdatafile, '','CERN-DST-EOS' )

job.setJobGroup( "Test_Run" )
job.setName( "Test" )
job.setLogLevel("DEBUG")

# job.setNumberOfProcessors(8)

copy2('/home/jsmiesko/Work/FCC/FCCAnalyses/examples/FCCee/higgs/mH-recoil/mumu/analysis_stage1_dirac.py', './analysis_stage1_dirac.py')
job.setInputSandbox(['./analysis_stage1_dirac.py'])

genapp = GenericApplication()
genapp.setSetupScript("/cvmfs/sw-nightlies.hsf.org/spackages6/key4hep-stack/master-2023-01-13/x86_64-centos7-gcc11.2.0-opt/kbla5/setup.sh")
genapp.setScript("/cvmfs/sw-nightlies.hsf.org/spackages6/fccanalyses/commit.6044eee8975fe8a1ea9cfa4122b19b0efd5ceea0/x86_64-centos7-gcc11.2.0-opt/rwcli/bin/fccanalysis")
genapp.setArguments('run ./analysis_stage1_dirac.py')
job.append(genapp)

submitmode='wms'
if len(servicesList) > 0:
    submitmode= servicesList[0]
print('SUBMIT MODE:', submitmode)
print(job.submit(dIlc, mode=submitmode))
