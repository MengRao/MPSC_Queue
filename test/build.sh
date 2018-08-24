set -x
g++ -std=c++11 -O3 -o mpsc_test mpsc_test.cc -pthread
g++ -std=c++11 -O3 -o shm_mpsc_test shm_mpsc_test.cc -pthread
g++ -std=c++11 -O3 -o async_logging async_logging.cc Logging.cc LogStream.cc -pthread
g++ -std=c++11 -O3 -DUSE_SHM=1 -o shm_logging_client shm_logging_client.cc Logging.cc LogStream.cc -lrt
g++ -std=c++11 -O3 -DUSE_SHM=1 -o shm_logging_server shm_logging_server.cc Logging.cc LogStream.cc -lrt

