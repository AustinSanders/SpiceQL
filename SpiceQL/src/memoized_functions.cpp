
#include "memo.h"
#include "memoized_functions.h"

using namespace std;

namespace SpiceQL { 

  string Memo::getCacheDir() { 
    static string CACHE_DIRECTORY = "";

    if (CACHE_DIRECTORY == "") { 
      const char* cache_dir_char = getenv("SPICEQL_CACHE_DIR");
    
      string cache_dir; 
      
      if (cache_dir_char == NULL) { 
        string tempname = "spiceql-cache-" + gen_random(10);
        cache_dir = fs::temp_directory_path() / tempname / "spiceql_cache"; 
      }
      else { 
        cache_dir = cache_dir_char;
      }

      if (!fs::is_directory(cache_dir)) { 
        spdlog::debug("{} does not exist, attempting to create the directory", cache_dir);
        fs::create_directories(cache_dir); 
      }
      
      CACHE_DIRECTORY = cache_dir;
      spdlog::debug("Setting cache directory to: {}", CACHE_DIRECTORY);  
    }
    else { 
      spdlog::debug("Cache Directory Already Set: {}", CACHE_DIRECTORY);  
    }
    return CACHE_DIRECTORY;
  }


  vector<pair<double, double>> Memo::getTimeIntervals(string kpath) {
    ExpiringPersistantCache c(getCacheDir(), kpath);
    spdlog::trace("Calling getTimeIntervals via cache located at {}", getCacheDir());
    static auto func_memoed = make_memoized(c, "spiceql_getTimeIntervals", SpiceQL::getTimeIntervals);
    return func_memoed(kpath); 
  }

  vector<string> Memo::ls(string const & root, bool recursive) {
    ExpiringPersistantCache c(getCacheDir(), root);
    spdlog::trace("Calling ls via cache located at {}", getCacheDir());
    static auto func_memoed = make_memoized(c, "spiceql_ls", SpiceQL::ls);
    return func_memoed(root, recursive);
  }
}