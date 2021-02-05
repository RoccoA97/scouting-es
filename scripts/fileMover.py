#! /usr/bin/python

if __name__ == "__main__":
    from os import listdir
    from os.path import isfile, join
    import time
    import os

    while True:
        try:
            onlyfiles = [f for f in listdir('/fff/ramdisk/scdaq') if isfile(join('/fff/ramdisk/scdaq', f)) and f.endswith('.dat')]
            if len(onlyfiles)==0:
                print "waiting for new file..."
                time.sleep(30)
            print onlyfiles
            for f in onlyfiles:
                full_filename = join('/fff/ramdisk/scdaq', f)
                outfile = f+'.bz2'
                dest_name = join('/fff/output/scdaq',outfile)
                command = "lbzip2 "+full_filename+" -c > "+dest_name
                print "compressing "+full_filename
                retval = os.system(command)
                if retval==0:
                    command = "rm "+full_filename
                    print "deleting "+full_filename
                    retval = os.system(command)
        except OSError as err:
            print err
            time.sleep(600)
