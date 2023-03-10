
#include "memo.h"
#include "memoized_functions.h"

#include "spice_types.h"

using namespace std;

namespace SpiceQL {

  vector<pair<double, double>> Memo::getTimeIntervals(string kpath) {
    Cache c({kpath});
    static auto func_memoed = make_memoized(c, "spiceql_getTimeIntervals", SpiceQL::getTimeIntervals);
    return func_memoed(kpath); 
  }


  vector<string> Memo::ls(string const & root, bool recursive) {
    Cache c({root});
    SPDLOG_TRACE("Calling ls via cache");
    static auto func_memoed = make_memoized(c, "spiceql_ls", SpiceQL::ls);
    return func_memoed(root, recursive);
  }


  int Memo::translateNameToCode(string frame, string mission, bool searchKernels) {
    Cache c({getDataDirectory()});
    spdlog::trace("Calling translateNameToCode via cache");
    static auto func_memoed = make_memoized(c, "spiceql_translateNameToCode", SpiceQL::translateNameToCode);
    return func_memoed(frame, mission, searchKernels);
  }


  string Memo::translateCodeToName(int frame, string mission, bool searchKernels) {
    Cache c({getDataDirectory()});
    spdlog::trace("Calling translateCodeToName via cache");
    static auto func_memoed = make_memoized(c, "spiceql_translateCodeToName", SpiceQL::translateCodeToName);
    return func_memoed(frame, mission, searchKernels);
  }
}