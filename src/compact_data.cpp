#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <atomic>
#include <vector>
#include <thread>
#include <future>

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



    atomic<bool> stop = false;
    promise<vector<DataUtil::DataPoint>> promise;
    future<vector<DataUtil::DataPoint>> future = promise.get_future();
    
    std::thread watcher([&](std::promise<std::vector<DataUtil::DataPoint>> &&p, const std::string &dir, std::atomic<bool> &stopFlag) {
        // Call watchDirectory and return its result via promise
        auto result = DataUtil::watchDirectory(dir, stopFlag);
        p.set_value(result); // Pass the result to the promise
    }, std::move(promise), rawDirectory, std::ref(stop));


    // dummy stop control for now
    std::cout << "Press Enter to stop watching..." << std::endl;
    std::cin.get(); 
    stop.store(true);

    watcher.join();
    auto compactDataList = future.get();
    vector<DataUtil::DataPoint> sortedCompact = DataUtil::sortDataList(compactDataList);
 
    DataUtil::partitionToOutput(sortedResultFile, sortedCompact, false);    

    return 0;
}