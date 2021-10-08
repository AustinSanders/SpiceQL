/**
  * @file
  *
  *
 **/

#include <fmt/format.h>
#include <SpiceUsr.h>

#include <nlohmann/json.hpp>

#include <ghc/fs_std.hpp>

#include "spice_types.h"
#include "query.h"
#include "utils.h"

using namespace std;
using json = nlohmann::json;

namespace SugarSpice {

  /**
   * @brief Used here to do reverse lookups of enum stringss
   **/
  template < typename T> pair<bool, int > findInVector(const std::vector<T>  & vecOfElements, const T  & element) {
    pair<bool, int > result;
    auto it = find(vecOfElements.begin(), vecOfElements.end(), element);
    if (it != vecOfElements.end()) {
      result.second = distance(vecOfElements.begin(), it);
      result.first = true;
    }
    else {
      result.first = false;
      result.second = -1;
    }
    return result;
  }


  const std::vector<std::string> Kernel::TYPES =  { "na", "ck", "spk", "tspk",
                                                    "lsk", "mk", "sclk",
                                                    "iak", "ik", "fk",
                                                    "dsk", "pck", "ek"};

  const std::vector<std::string> Kernel::QUALITIES = { "na",
                                                       "predicted",
                                                       "nadir",
                                                       "reconstructed",
                                                       "smithed"};


  string Kernel::translateType(Kernel::Type type) {
    return Kernel::TYPES[static_cast<int>(type)];
  }


  Kernel::Type Kernel::translateType(string type) {
    auto res = findInVector<string>(Kernel::TYPES, type);
    if (res.first) {
      return static_cast<Kernel::Type>(res.second);
    }

    throw invalid_argument(fmt::format("{} is not a valid kernel type", type));
  };


  string Kernel::translateQuality(Kernel::Quality qa) {
    return Kernel::QUALITIES[static_cast<int>(qa)];
  }

  Kernel::Quality Kernel::translateQuality(string qa) {
    auto res = findInVector<string>(Kernel::QUALITIES, qa);
    if (res.first) {
      return static_cast<Kernel::Quality>(res.second);
    }

    throw invalid_argument(fmt::format("{} is not a valid kernel type", qa));
  }


  int Kernel::translateFrame(string frame) {
    SpiceInt code;
    SpiceBoolean found;

    bodn2c_c(frame.c_str(), &code, &found);

    if (!found) {
      throw "Frame name not Found";
    }

    return code;
  }


  string Kernel::translateFrame(int frame) {
    SpiceChar name[128];
    SpiceBoolean found;

    bodc2n_c(frame, 128, name, &found);

    if(!found) {
      throw "Frame Code not found";
    }

    return string(name);
  }


  Kernel::Kernel(string path) {
    KernelPool::load(path, true);
  }


  Kernel::~Kernel() {
    KernelPool::unload(this->path);
  }


  double utcToEt(string utc) {
      // get lsk kernel
      json conf = getMissionConfig("base");
      conf = globKernels(getDataDirectory(), conf, "lsk");
      Kernel lsk(getLatestKernel(conf.at("base").at("lsk").at("kernels")));

      SpiceDouble et;
      utc2et_c(utc.c_str(), &et);
      return et;
  }

  std::unordered_map<std::string, int> KernelPool::refCounts;
  
  int KernelPool::load(string path, bool force_refurnsh) {
    int refCount; 
    auto it = refCounts.find(path);

    if (it != refCounts.end()) {
      // it's been furnished before, increment ref count
      it->second += 1;
      refCount = it->second; 

      if (force_refurnsh) {
        furnsh_c(path.c_str());
      } 
    }
    else {  
      furnsh_c(path.c_str());
      // load the kernel and register in onto the kernel map 
      refCounts.emplace(path, 1);
    }

    return refCount;
  }


  int KernelPool::unload(string path) {
    try { 
      int &refcount = refCounts.at(path);
      
      // if the map contains the last copy of the kernel, delete it
      if (refcount == 1) {
        // unfurnsh the kernel
        unload_c(path.c_str());
        int ndeleted = refCounts.erase(path);
        return 0;
      }
      else {
        refcount--;
        return refcount;
      }
    }
    catch(out_of_range &e) {
      throw out_of_range(path + " is not a kernel that has been loaded."); 
    }
  }


  unsigned int KernelPool::refCount(std::string key) {
    try {
      return refCounts.at(key);
    } catch(out_of_range e&) {
      return 0;
    }
  }


  map<string, int> refCounts() {
    return refMap;
  }


  KernelSet::KernelSet(json kernels) {
      this->kernels = kernels; 

      vector<json::json_pointer> pointers = findKeyInJson(kernels, "kernels", true);

      for(auto &p : pointers) { 
        json jkernels = kernels[p]; 
        vector<Kernel> res; 
        for(auto & k : jkernels) { 
          res.emplace_back(Kernel(k));
        }
        loadedKernels.emplace(p, res);
      } 
  }
} 
 