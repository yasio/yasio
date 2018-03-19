#!/bin/bash

echo welcome to use os x boost asio auto extract tool!

boost_dir=/projects/inetlibs/boost_1_66_0
boost_asio_dir=/projects/inetlibs/boost.asio.1.0.166

if [ ! -d $boost_asio_dir ]; then
  mkdir $boost_asio_dir
fi

echo $boost_dir
error_code=1
while (($error_code != 0))
do
  loop=$((loop + 1))
  
  gcc -std=c++11 pseudo.cpp -c -I$boost_asio_dir -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_SYSTEM_NO_LIB -DBOOST_DATE_TIME_NO_LIB -DBOOST_REGEX_NO_LIB -DBOOST_ASIO_DISABLE_THREADS 2>compile_error.txt
  error_code=$?
  error_msg=`cat compile_error.txt | grep "fatal error"` 
  echo error_msg is: $error_msg 
  echo error_code is: $error_code
  if (($error_code != 0)); then
    missing=`echo "$error_msg" | cut -d "'" -f 2`
    missing_src=$boost_dir/$missing
    missing_dest=$boost_asio_dir/`dirname $missing` 
    
    echo missing src:$missing_src
    echo missing dest:$missing_dest
    if [ ! -d $missing_dest ]; then
      mkdir -p $missing_dest
    fi
    cp -f $missing_src $missing_dest/
  fi  

done

echo Congratulations, extract boost.asio succeed, enjoy it.

#end

