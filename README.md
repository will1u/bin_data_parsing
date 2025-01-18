data_readout.cpp: sends raw binary data to server

receive_packet.cpp: receives raw data and writes to disk (../data_parsing/partitioned_data)

compact_data.cpp: scans ../data_parsing/partitioned_data for changes. then builds each and compacts into result file in ../data_parsing/combined_data

included 256 test files in tests