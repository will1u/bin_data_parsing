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
    string resultFile = resultDirectory + "combined_data_" + TimingUtil::getTime() + ".txt";

    DataUtil::watchDirectory(rawDirectory, resultFile);
    return 0;
}