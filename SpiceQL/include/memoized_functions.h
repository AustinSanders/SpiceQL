
#include <vector>
#include <string>

#include "utils.h"

namespace SpiceQL {
  namespace  Memo {

    std::string getCacheDir();

    /**
      * @brief ls, like in unix, kinda. Also it's a function. This 
      * is memoized so it'll load from cache if run multiple times with the same 
      * parameters. 
      *
      * Iterates the input path and returning a list of files. Optionally, recursively.
      *
      * @param root The root directory to search
      * @param recursive recursively iterates through directories if true
      *
      * @returns list of paths
    **/
    std::vector<std::string> ls(std::string const & root, bool recursive);
    
  
  /**
    * @brief Get start and stop times a kernel.
    *
    * For each segment in the kernel, get all start and stop times as a vector of double pairs.
    * This gets all start and stop times regardless of the frame associated with it.
    *
    * Input kernel is assumed to be a binary kernel with time dependant external orientation data.
    *
    * @param kpath Path to the kernel
    * @returns std::vector of start and stop times
   **/
    std::vector<std::pair<double, double>> getTimeIntervals(std::string kpath);

  /**
    * @brief Memoized wrapper for translateNameToCode
    * 
    * Captures the result from a translateNameToCode call to speed up
    * subsequent calls of the same function call
    *
    * @see SpiceQL::Kernel::translateNameToCode
    *
    * @param frame Name of frame to translate
    * @param mission Name of mission
    * @returns int
   **/
    int translateNameToCode(std::string frame, std::string mission);

  /**
    * @brief Memoized wrapper for translateCodeToName
    * 
    * Captures the result from a translateCodeToName call to speed up
    * subsequent calls of the same function call
    * 
    * @see SpiceQL::Kernel::translateCodeToName
    *
    * @param frame Code of frame to translate
    * @param mission Name of mission
    * @returns std::string
   **/
    std::string translateCodeToName(int frame, std::string mission);
  }
}
