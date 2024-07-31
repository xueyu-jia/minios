#!/bin/bash
# 用于快速生成测试程序的脚本 jiangfeng 2024.7

DEFAULT_TEST_DIR=user/user/test/
RUN_SCRIPT="run.sh"
TEST_DIR=$DEFAULT_TEST_DIR
# 如果传递参数，则取第一个参数为测试程序的根文件夹
if [[ "$#" -gt "0" ]]; then
    TEST_DIR=$1
fi
# makeScript dirname
makeScript(){
    # echo $1
    cd "$1"
    if [[ -e ./$RUN_SCRIPT ]]; then
        rm ./$RUN_SCRIPT
    fi
    for dir in `find ./* -maxdepth 0 -type d`
    do
        echo "cd $dir" >> $RUN_SCRIPT
        echo "$RUN_SCRIPT" >> $RUN_SCRIPT
        echo "cd .." >> $RUN_SCRIPT
        makeScript $dir
    done
    for file in `find ./* -maxdepth 0  -name '*.c' -type f`
    do
        echo ${file%\.c} >> $RUN_SCRIPT
    done
    cd ..
}
makeScript $TEST_DIR