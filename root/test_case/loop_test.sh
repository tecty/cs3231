#!/bin/bash

for (( i=0; i < 10; i++ )); do
  # echo the test situation
  echo ""
  echo "---------------------------------"
  echo "test of seed : "$i


  # create the conf file by apply new random seed
  cat sys161.conf.tmplate  > sys161.conf
  echo '28      random  seed='$i >> sys161.conf

  sh $1
done
