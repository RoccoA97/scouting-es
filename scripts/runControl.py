#! /usr/bin/python



if __name__ == "__main__":

    import urllib2
    import json
    import time


    import socket
    import sys

    scouting_running = False
    current_run = 0;
    while True:
        data=json.loads(urllib2.urlopen("http://daq-expert.cms:8081/DAQSnapshotService/getsnapshot?setup=cdaq").read())
        runNumber = data['runNumber']
        beamMode = data['lhcBeamMode']
        daqState = data['daqState']
        print "daq state ",daqState," beamMode ",beamMode," runNumber ",runNumber
        #if not scouting_running:
	if True:
            if beamMode=='STABLE BEAMS' and daqState=='Running':
                print "going to start scouting for run ",runNumber
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server_address = ('localhost', 8000)
                print >>sys.stderr, 'connecting to %s port %s' % server_address
                try:
                    sock.connect(server_address)
                    message = "start "+str(runNumber)
                    sock.sendall(message)
                    data = sock.recv(16)
                    print "server response ",data
                    if 'ok' in data:
                        current_run=runNumber
                        print "started run ",current_run
                        scouting_running = True
                    sock.close()

                except:
                    print "error in connecting to scout daq"

	if True:
        #if scouting_running:
            if beamMode!='STABLE BEAMS' or daqState != 'Running':
                print "going to stop scouting for run ", current_run
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                server_address = ('localhost', 8000)
                print >>sys.stderr, 'connecting to %s port %s' % server_address
                try:
                    sock.connect(server_address)
                    message = "stop"
                    sock.sendall(message)
                    data = sock.recv(16)
                    print "server response ",data
                    if 'ok' in data:
                        print "stopped run ",current_run
                        current_run = 0
                        scouting_running = False
                    sock.close()

                except:
                    print "error in connecting to scout daq"

        time.sleep(30)
