%module(package="pyspiceql") memoized_functions


%{
  #include "memoized_functions.h"
%}

%rename(Memo_ls) SpiceQL::Memo::ls;
%rename(Memo_getTimeIntervals) SpiceQL::Memo::getTimeIntervals;
%rename(Memo_globTimeIntervals) SpiceQL::Memo::globTimeIntervals;

%include "memoized_functions.h"
