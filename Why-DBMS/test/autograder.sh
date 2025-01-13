#!/bin/bash

#Settings
temp_file="temp.txt"
full_score=0
score=0
style_score=0

if [ $# -eq 1 ]; then
    path=$1
else
    path="../"
fi

echo -e "\n=====================CHECK======================\n"

if python3 --version &> /dev/null -ne 0; then 
    echo "Python3 is not installed"
    exit -1
fi

if !pip --version &> /dev/null -ne 0; then
    echo "pip is not installed"
    exit -1
fi

echo -e "\n=====================STYLE======================\n"

pip install cpplint
if cpplint ${path}*.cc ${path}*.h > ${temp_file} -eq 0; then 
    style_score=20
else
    python3 style_checker.py ${temp_file} 0 #give basic score of 0
    style_score=$?
fi
rm -rf ${temp_file}

echo -e "\n=====================DONE======================\n"

echo -e "\n=====================BUILD======================\n"

(cd ${path}; make clean; make)

echo -e "\n====================BUILD DONE==================\n"

echo -e "\n===================AUTOGRADING==================\n"

echo "Checking query 1"

for test_folder in q1/*/; do
    echo -e "\n-----Run ${test_folder}-----"

    ${path}project1 q1 ${test_folder}customer.txt ${test_folder}zonecost.txt > ${temp_file}

    full_score=$( expr ${full_score} + 1 )
    test_name=${test_folder}

    if diff -b ${temp_file} ${test_folder}result.txt &>/dev/null; then
        echo -e "\033[32m"PASS"\033[0m"
        score=$( expr ${score} + 1 )
    else
        echo -e "\033[31m"FAIL"\033[0m"
    fi

    echo -e "-------Done-------"
done

echo -e "\nChecking query 2"

for test_folder in q2/*/; do
    echo -e "\n-----Run ${test_folder}-----"

    ${path}project1 q2 ${test_folder}lineitem.txt ${test_folder}products.txt > ${temp_file}

    full_score=$( expr ${full_score} + 1 )
    test_name=${test_folder}

    if diff -b ${temp_file} ${test_folder}result.txt &>/dev/null; then
        echo -e "\033[32m"PASS"\033[0m"
        score=$( expr ${score} + 1 )
    else
        echo -e "\033[31m"FAIL"\033[0m"
    fi

    echo -e "-------Done-------"
done

rm -rf ${temp_file}

echo -e "\n====================TEST SCORE===================\n"

conversion_score=$( expr ${score} \* 80 / ${full_score} + ${style_score} )
echo "PASSED : ${score}"
echo "TOTAL  : ${full_score}"
echo "STYLE  : ${style_score}"
echo -e "\nSCORE  : ${conversion_score}/100"

echo -e "\n"

exit ${conversion_score}
