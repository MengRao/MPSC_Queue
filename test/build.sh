set -x
g++ -std=c++11 -O3 -o speed_test speed_test.cc -pthread
g++ -std=c++11 -O3 -o async_logging async_logging.cc Logging.cc LogStream.cc -pthread

