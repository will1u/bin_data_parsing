data_readout.cpp: sends raw binary data to server

receive_packet.cpp: receives raw data and writes to disk (../bin/partitioned_data)

compact_data.cpp: scans ../data_parsing/partitioned_data for changes. then builds each and compacts into result file in ../data_parsing/combined_data THIS IS NOT RIGHT NEEDS TO CHANGE TO SCANNING /BIN/PARTIIONED_DATA

included 256 test files in tests
