/**
 * Implements an almost transparent disk cache/memoization for C++ functions.
 *
 * Published under three-clause BSD license.
 * Copyright 2014 Hannes Schulz <schulz@ais.uni-bonn.de>
 */
#ifndef memo_h
#define memo_h

#include <map>
#include <fstream>
#include <utility>
#include <functional>
#include <any>
#include <string>

#include <ghc/fs_std.hpp>
#include <spdlog/spdlog.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/utility.hpp>

#include "memoized_functions.h"

#define CACHED(cache, func, ...) cache(#func, func, __VA_ARGS__)


namespace SpiceQL {
namespace Memo {
    template <class T>
    inline size_t _hash_combine(std::size_t& seed, const T& v) {
        SPDLOG_TRACE("_hash_combine seed: {}", seed);

        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);

        return seed;
    }

    inline size_t hash_combine(std::size_t& seed) {
        return seed;
    }

    template <typename T, typename... Params>
    inline size_t hash_combine(std::size_t& seed, const T& t, const Params&... params) {
        seed = _hash_combine(seed, t);
        return hash_combine(seed, params...);
    }
    
    class ExpiringPersistantCache  { 
        public:
        // if cache is older than this directory, then the cache is reloaded
        std::string m_dependant; 
        std::string m_path;

        ExpiringPersistantCache(std::string path, std::string dep)
        :  m_dependant(dep), m_path(path) {
            fs::create_directories(m_path);
            SPDLOG_TRACE("Dep and path: {}, {}", m_dependant, m_path);
        }

        template<typename Func, typename... Params>
            auto operator()(const Func& f, Params&&... params) -> decltype(f(params...))const{
                return this->operator()("anonymous", f, std::forward<Params>(params)...);
        }

        template<typename Func, typename... Params>
            auto operator()(const std::string& descr, const Func& f, Params&&... params) -> decltype(f(params...))const {
                std::size_t seed = 0; 
                hash_combine(seed, descr, params...);
                return this->operator()(descr, seed, f, std::forward<Params>(params)...);
        }

        template<typename Func, typename... Params>
            auto operator()(const std::string& descr, std::size_t seed, const Func& f, Params&&... params) -> decltype(f(params...))const{
                typedef decltype(f(params...)) retval_t;
                std::string name = descr + "-" + std::to_string(seed);
                
                SPDLOG_TRACE("Cache name: {}", name);

                std::string fn = (fs::path(m_path) / name).string();
                
                SPDLOG_TRACE("cache full path: {}", fn);

                if(fs::exists(fn) && fs::exists(m_dependant) && fs::last_write_time(fn) > fs::last_write_time(m_dependant)) {
                    SPDLOG_TRACE("Cached access of {}", name);
                    std::ifstream ifs(fn);
                    cereal::BinaryInputArchive  ia(ifs);
                    retval_t ret;
                    ia >> ret;
                    return ret;
                }
                else if (fs::exists(fn) && fs::exists(m_dependant) && fs::last_write_time(fn) <= fs::last_write_time(m_dependant)) { 
                    // delete and reload cache
                    SPDLOG_TRACE("{} is newer, {} has expired", m_dependant, fn);
                    fs::remove(fn);
                }
                // if dependant doesn't exist, treat it as a cache miss. Function might naturally fail.
                
                SPDLOG_TRACE("Non-cached access, creating cache {}", fn);
                retval_t ret = f(std::forward<Params>(params)...);
                std::ofstream ofs(fn);
                cereal::BinaryOutputArchive oa(ofs);
                oa << ret;

                return ret;
            }
    };

    class Memory {
        mutable std::map<std::size_t, std::any> m_data;

        template<typename Func, typename... Params>
            auto operator()(const Func& f, Params&&... params) -> decltype(f(params...)) const {
                return (*this)("anonymous", f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(std::string descr, const Func& f, Params&&... params) -> decltype(f(params...)) const {
                std::size_t seed = hash_combine(0, descr, params...);
                return (*this)(seed, f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(const std::string& descr, std::size_t seed, const Func& f, Params&&... params) -> decltype(f(params...)) const {
                hash_combine(seed, descr);
                return (*this)(seed, f, std::forward<Params>(params)...);
            }
        template<typename Func, typename... Params>
            auto operator()(std::size_t seed, const Func& f, Params&&... params) -> decltype(f(params...)) const {
                typedef decltype(f(params...)) retval_t;
                auto it = m_data.find(seed);
                if(it != m_data.end()){
                    SPDLOG_TRACE("Cached access from memory");
                    return std::any_cast<retval_t>(it->second);
                }
                retval_t ret = f(std::forward<Params>(params)...);
                SPDLOG_TRACE("Non-cached access");
                m_data[seed] = ret;
                return ret;
            }
    };


    template<typename Cache, typename Function>
    struct memoize{
        const Function m_func; // we require copying the function object here.
        std::string m_id;
        Cache m_fc;
        memoize(Cache& fc, std::string id, const Function& f)
            :m_func(f), m_id(id), m_fc(fc){}
        template<typename... Params>
        auto operator()(Params&&... args) 
                -> decltype(std::bind(m_func, args...)()) {
            SPDLOG_TRACE("Calling a memo func with ID: {}", m_id);
            return m_fc(m_id, m_func, std::forward<Params>(args)...);
        }
    };
      
    template<typename Cache, typename Function>
    memoize<Cache, Function>
    make_memoized(Cache& fc, const std::string& id, Function f){
        SPDLOG_TRACE("Making function with ID {}",id);   
        return memoize<Cache, Function>(fc, id, f);
    }
 
}
}

namespace std {
  // literally have no idea why I need to make this specialization... can't compile otherwise
  template <> struct hash<std::vector<std::string >(const std::string&, bool)>
  {
    size_t operator()(const auto & x) const {
      return 0;
    }
  };
}

#endif