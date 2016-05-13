#!/bin/bash

geog=`grep '"Geography"' config.json | awk '{ print $(NF) }'| sed 's/"\(.*\)",/\1/g'`
#echo $geog
config_name=`grep Config_Name config.json | awk '{ print $(NF) }'| sed 's/"\(.*\)",/\1/g'`
#echo $config_name

if [ ! -n "$EMOD_ROOT" ]
then
    echo "EMOD_ROOT is not set."
    exit
fi
$EMOD_ROOT/build/x64/Release/Eradication/Eradication -C config.json -O testing -I $EMOD_ROOT/InputData/$geog
python $EMOD_ROOT/Regression/plotAllCharts.py output/InsetChart.json testing/InsetChart.json $config_name

