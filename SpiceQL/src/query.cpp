/**
  * @file
  *
  *
  *
  *
 **/
#include <fstream>
#include <algorithm>

#include <SpiceUsr.h>

#include <ghc/fs_std.hpp>
#include <spdlog/spdlog.h>

#include <spdlog/spdlog.h>

#include "query.h"
#include "spice_types.h"
#include "utils.h"
#include "memoized_functions.h"
#include "config.h"

using json = nlohmann::json;
using namespace std;

namespace SpiceQL {


 std::string getKernelStringValue(std::string key) {
   // check to make sure the key exists when calling findKeyWords(key)
   if (findKeywords(key).contains(key)){
      json results = findKeywords(key);
      std::string keyResult;
      if (results[key].is_string()) {
          keyResult = results[key];
      }
      else {
          keyResult = results[key].dump();
      }
      return keyResult;
    }
    // throw exception
    else{
      throw std::invalid_argument("key not in results");
    }
  }

  std::vector<string> getKernelVectorValue(std::string key) {

    // check to make sure the key exists when calling findKeyWords(key)
    if (findKeywords(key).contains(key)){

      // get json results of key
      json results = findKeywords(key);
      vector<string> kernelValues;

      // iterate over results @ key
      for(auto i : results[key]){
        // push values to vector
        kernelValues.push_back(to_string(i));
      }
      return kernelValues;
    }
    // throw exception
    else{
      throw std::invalid_argument("key not in results");
    }
  }

  json findMissionKeywords(string key, string mission) {
    Config missionConf;
    json globalConf = missionConf.globalConf();
    json j;
    if (globalConf.find(mission) != globalConf.end()) {
      vector<string> kernelsToGet = {"ik", "iak"};
      j = missionConf[mission].get(kernelsToGet);
      json missionKernels = {};
      missionKernels["ik"] = j["ik"];
      missionKernels["iak"] = j["iak"];
      j = getLatestKernels(missionKernels);
    }
    else {
      throw invalid_argument(fmt::format("Could not find mission: \"{}\" in config.", mission));
    }
    KernelSet kset(j);

    // check to make sure the key exists when calling findKeywords(key)
    if (findKeywords(key).contains(key)){
      return findKeywords(key);
    }
    // throw exception
    else {
      throw invalid_argument(fmt::format("key: {} not in kernels for mission: {}", key, mission));
    }
  }


  vector<string> getLatestKernel(vector<string> kernels) {
    if(kernels.empty()) {
      throw invalid_argument("Can't get latest kernel from empty vector");
    }
    vector<vector<string>> files = {};

    string extension = static_cast<fs::path>(kernels.at(0)).extension();
    transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    // ensure everything is different versions of the same file
    for(const fs::path &k : kernels) {
      string currentKernelExt = k.extension();
      transform(currentKernelExt.begin(), currentKernelExt.end(), currentKernelExt.begin(), ::tolower);
      if (currentKernelExt != extension) {
        throw invalid_argument("The input extensions (" + (string)k.filename() + ") are not different versions of the same file " + kernels.at(0));
      }
      bool foundList = false;
      for (int i = 0; i < files.size(); i++) {
        const fs::path &firstVecElem = files[i][0];
        string fileName = firstVecElem.filename();
        string kernelName = k.filename();
        int findRes = fileName.find_first_of("0123456789");
        if (findRes != string::npos) {
          fileName = fileName.erase(findRes);
        }
        findRes = kernelName.find_first_of("0123456789");
        if (findRes != string::npos) {
          kernelName = kernelName.erase(findRes);
        }
        if (fileName == kernelName) {
          files[i].push_back(k);
          foundList = true;
        }
      }
      if (!foundList) {
        files.push_back({k});
      }
      foundList = false;
    }

    vector<string> outKernels = {};
    for (auto kernelList : files) {
      outKernels.push_back(*(max_element(kernelList.begin(), kernelList.end())));
    }

    return outKernels;
  }


  json getLatestKernels(json kernels) {

    vector<json::json_pointer> kptrs = findKeyInJson(kernels, "kernels", true);
    vector<vector<string>> lastest; 

    for (json::json_pointer &ptr : kptrs) {  
      vector<vector<string>> kvect = json2DArrayTo2DVector(kernels[ptr]);
      vector<vector<string>> newLatest;
 

      for (auto &vec : kvect) {
        vector<string> latest = getLatestKernel(vec);
        SPDLOG_TRACE("Adding Kernels To Latest: {}", fmt::join(latest, ", "));
        newLatest.push_back(latest);
      }
      kernels[ptr] = newLatest;
    }

    return kernels;     
  }


  json globKernels(string root, json conf, string kernelType) {
    SPDLOG_TRACE("globKernels({}, {}, {})", root, conf.dump(), kernelType);
    vector<json::json_pointer> pointers = findKeyInJson(conf, kernelType, true);

    json ret;

    // iterate pointers
    for(auto pointer : pointers) {
      json category = conf[pointer];

      if (category.contains("kernels")) {
        ret[pointer]["kernels"] = getPathsFromRegex(root, category.at("kernels"));
      }

      if (category.contains("deps")) {
        if (category.at("deps").contains("sclk")) {
          ret[pointer]["deps"]["sclk"] = getPathsFromRegex(root, category.at("deps").at("sclk"));
        }
        if (category.at("deps").contains("pck")) {
          ret[pointer]["deps"]["pck"] = getPathsFromRegex(root, category.at("deps").at("pck"));
        }
        if (category.at("deps").contains("objs")) {
          ret[pointer]["deps"]["objs"] = category.at("deps").at("objs");
        }
      }

      // iterate over potential qualities
      for(auto qual: Kernel::QUALITIES) {
        if(!category.contains(qual)) {
          continue;
        }

        vector<vector<string>> binKernels = getPathsFromRegex(root, category[qual].at("kernels"));
        if (!binKernels.empty()) {
          ret[pointer][qual]["kernels"] = binKernels;
        } 

        if (category[qual].contains("deps")) {
          if (category[qual].at("deps").contains("sclk")) {
            ret[pointer][qual]["deps"]["sclk"] = getPathsFromRegex(root, category[qual].at("deps").at("sclk"));
          }
          if (category[qual].at("deps").contains("pck")) {
            ret[pointer][qual]["deps"]["pck"] = getPathsFromRegex(root, category[qual].at("deps").at("pck"));
          }
          if (category[qual].at("deps").contains("objs")) {
            ret[pointer][qual]["deps"]["objs"] = category[qual].at("deps").at("objs");
          }
        }
      }
    }

    SPDLOG_DEBUG("Kernels To Search: {}", conf.dump());
    SPDLOG_DEBUG("Kernels: {}", ret.dump());
    return  ret.empty() ? "{}"_json : ret;
  }


  json listMissionKernels(string root, json conf) {
    json kernels;

    // the kernels group is now the conf with
    for(auto &kernelType: {"ck", "spk", "tspk", "fk", "ik", "iak", "pck", "lsk", "sclk"}) {
        kernels.merge_patch(globKernels(root, conf, kernelType));
    }
    return kernels;
  }


  json searchMissionKernels(json kernels, std::vector<double> times, bool isContiguous)  {
    json reducedKernels;
    SPDLOG_TRACE("In searchMissionKernels");

    // Load any SCLKs in the config
    vector<KernelSet> sclkKernels;
    for (auto &p : findKeyInJson(kernels, "sclk", true)) {
      sclkKernels.push_back(KernelSet(kernels[p]));
    }

    vector<json::json_pointer> ckpointers = findKeyInJson(kernels, "ck", true);
    vector<json::json_pointer> spkpointers = findKeyInJson(kernels, "spk", true);
    vector<json::json_pointer> pointers(ckpointers.size() + spkpointers.size());
    merge(ckpointers.begin(), ckpointers.end(), spkpointers.begin(), spkpointers.end(), pointers.begin());

    json newKernels = json::array();

    // refine cks for every instrument/category
    for (auto &p : pointers) {
      json cks = kernels[p];
      SPDLOG_TRACE("In searchMissionKernels: searching for {}", cks.dump());

      if(cks.is_null() ) {
        continue;
      }

      for(auto qual: Kernel::QUALITIES) {
        if(!cks.contains(qual)) {
          continue;
        }

        json ckQual = cks[qual]["kernels"];
        newKernels = json::array();

        for(auto &subArr : ckQual) {
          for (auto &kernel : subArr) {
            json newKernelsSubArr = json::array();

            vector<pair<double, double>> intervals = Memo::getTimeIntervals(kernel);
            for(auto &interval : intervals) {
              auto isInRange = [&interval](double d) -> bool {return d >= interval.first && d <= interval.second;};

              if (isContiguous && all_of(times.cbegin(), times.cend(), isInRange)) {
                newKernelsSubArr.push_back(kernel);
              }
              else if (any_of(times.cbegin(), times.cend(), isInRange)) {  
                newKernelsSubArr.push_back(kernel);
              }
            } // end of searching subarr
            
            SPDLOG_TRACE("kernel list found: {}", newKernelsSubArr.dump());
            if (!newKernelsSubArr.empty()) {
              newKernels.push_back(newKernelsSubArr);
            }
          } // end  of searching arr
        } // end of iterating qualities
        
        SPDLOG_TRACE("newKernels {}", newKernels);
        reducedKernels[p/qual/"kernels"] = newKernels;
        reducedKernels[p]["deps"] = kernels[p]["deps"];
      }
    }
    kernels.merge_patch(reducedKernels);
    return kernels;
  }


  json listMissionKernels(json conf) {
    fs::path root = getDataDirectory();
    return searchMissionKernels(root, conf);
  }


  vector<string> getKernelsAsVector(json kernels) {
    SPDLOG_TRACE("geKernelsAsVector json: {}}", kernels.dump());

    vector<json::json_pointer> pointers = findKeyInJson(kernels, "kernels");
    vector<string> kernelVect;

    if (pointers.empty() && kernels.is_array()) {
      vector<vector<string>> ks = json2DArrayTo2DVector(kernels);
      for (auto &subarr : ks) { 
        kernelVect.insert(kernelVect.end(), subarr.begin(), subarr.end());
      } 
    }
    else {
      for (auto & p : pointers) {
        vector<vector<string>> ks = json2DArrayTo2DVector(kernels[p]);
        for (auto &subarr : ks) { 
          kernelVect.insert(kernelVect.end(), subarr.begin(), subarr.end());
        }
      }    
    }
    return kernelVect;
  }


  set<string> getKernelsAsSet(json kernels) {
    vector<json::json_pointer> pointers = findKeyInJson(kernels, "kernels");

    set<string> kset;
    
    if (pointers.empty() && kernels.is_array()) {
      vector<vector<string>> ks = json2DArrayTo2DVector(kernels);
      for (auto &subarr : ks) {
        for (auto &k : subarr) {
          kset.emplace(k);
        }
      }
    }
    else {
      for (auto & p : pointers) {
        vector<vector<string>> ks = json2DArrayTo2DVector(kernels[p]);
        for (auto &subarr : ks) {
          for (auto &k : subarr) {
            kset.emplace(k);
          }
        }
      }
    }    

    return kset;
  }

}
