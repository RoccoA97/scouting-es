# scouting-es
Simple set of classes to collect data from scouting and save to files

Compile with 

g++ -std=c++0x -g -ltbb -lboost_thread -lcurl \*.cc

example conf in scdaq.conf:


input_file:/dev/shm/testdata.bin  
output_filename_base:/tmp/scdaq  
max_file_size:1073741824  
threads:8  
input_buffers:20  
blocks_buffer:1000  
port:8000  
elastic_url:no   
pt_cut:7  
</code>
