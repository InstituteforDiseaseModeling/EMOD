#!/bin/bash

echo "Running locally."

if [ ! -n "$EMOD_ROOT" ]
then
    echo "EMOD_ROOT is not set."
    exit
fi

echo "Running YAML demo."
$EMOD_ROOT/build/x64/Release/Eradication/Eradication -C config.yaml -O testing_json -I $EMOD_ROOT/Regression/Scenarios/InputFiles -P .
echo "Running multi-json demo."
$EMOD_ROOT/build/x64/Release/Eradication/Eradication -C config_lite.json -O testing_json -I $EMOD_ROOT/Regression/Scenarios/InputFiles -P .
echo "Running MS Excel demo."
$EMOD_ROOT/build/x64/Release/Eradication/Eradication -C config.xlsx -O testing_json -I $EMOD_ROOT/Regression/Scenarios/InputFiles -P .


