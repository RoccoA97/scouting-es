#! /usr/bin/python

## ---- Configuration ----

# Delay between two consecutive run control polls
pollingDelay = 10

# Acquire data in the following beam modes and DAQ states:
allowedBeamModes = ['STABLE BEAMS', 'ADJUST']
allowedDaqStates = ['RUNNING', 'RUNNINGDEGRADED']

## -----------------------


if __name__ == "__main__":

    import urllib2
    import json
    import time


    import socket
    import sys

    current_run = 0;
    while True:
        data=json.loads(urllib2.urlopen("http://daq-expert.cms:8081/DAQSnapshotService/getsnapshot?setup=cdaq").read())
        runNumber = data['runNumber']
        beamMode = data['lhcBeamMode'].upper()
        daqState = data['daqState'].upper()
        print "daq state ",daqState," beamMode ",beamMode," runNumber ",runNumber

        if True:
            # Check if LHC is running
            if (beamMode in allowedBeamModes) and (daqState in allowedDaqStates):
                print "going to start scouting for run ",runNumber
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server_address = ('localhost', 8000)
                print >>sys.stderr, 'connecting to %s port %s' % server_address
                try:
                    sock.connect(server_address)
                    message = "start "+str(runNumber)
                    sock.sendall(message)
                    data = sock.recv(256)
                    print "server response ",data
                    if 'ok' in data:
                        current_run=runNumber
                        print "started run ",current_run
                        scouting_running = True
                    sock.close()

                except:
                    print "error in connecting to scout daq"

        if True:
            # Check if LHC is not running
            if (beamMode not in allowedBeamModes) or (daqState not in allowedDaqStates):
                print "going to stop scouting for run ", current_run
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server_address = ('localhost', 8000)
                print >>sys.stderr, 'connecting to %s port %s' % server_address
                try:
                    sock.connect(server_address)
                    message = "stop"
                    sock.sendall(message)
                    data = sock.recv(256)
                    print "server response ",data
                    if 'ok' in data:
                        print "stopped run ",current_run
                        current_run = 0
                        scouting_running = False
                    sock.close()

                except:
                    print "error in connecting to scout daq"

        time.sleep(pollingDelay)
