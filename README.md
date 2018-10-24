# scouting-es
Simple set of classes and scripts to collect data from scouting prototype and save to files.

## Build

~~**Compile with**~~ 

~~g++ -std=c++0x -g -ltbb -lboost_thread -lcurl \*.cc~~

**Use supplied Makefile:**

```
$ cd src
$ make
```

## Run

1. Start the Vivado reset server on `scoutsrv`:
    ```
    $ cd scripts/vivado
    $ ./reset_server.sh
    ```

2. Start the file mover on `bu`:
    ```
    $ cd scripts
    $ ./fileMover.py
    ```

3. Start the run control on `scoutdaq`:
    ```
    $ cd scripts
    $ ./runControl.py
    ```

4. Start the data taking application on `scoutdaq`:
    ```
    $ cd scripts
    $ ./run.sh
    ```

## Configuration

#### example conf in scdaq.conf:

```
input_file:/dev/shm/testdata.bin  
output_filename_base:/tmp/scdaq  
max_file_size:1073741824  
threads:8  
input_buffers:20  
blocks_buffer:1000  
port:8000  
elastic_url:no   
pt_cut:7  
```
