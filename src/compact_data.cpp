#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include "data_util.h"
#include "directory_util.h" 
#include "timing_util.h"

using namespace std;


int main() {
    
    string rawDirectory = "~/projects/data_parsing/partitioned_data/";
    rawDirectory = DirectoryUtil::expandTilde(rawDirectory);
    string resultDirectory = "~/projects/data_parsing/combined_data/";
    resultDirectory = DirectoryUtil::expandTilde(resultDirectory); 
    string sortedResultFile = resultDirectory + "sorted_combined_data_" + TimingUtil::getTime() + ".txt";

    //watchDirectory needs to have some kind of stop? or maybe i just sort as it goes
    vector<DataUtil::DataPoint> compactDataList = DataUtil::watchDirectory(rawDirectory);
    DataUtil::partitionToOutput(sortedResultFile, compactDataList, false);    

    return 0;
}