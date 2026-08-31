#!/bin/bash
# ============================================
#  Keil 编译 & 烧录脚本 (Git Bash)
#  用法: ./run.sh [-b | -f | -a]
#  无参数默认: 编译 + 烧录
#  新电脑只需修改下面两行路径即可
# ============================================
#UV="/d/Soft/KEIL5/UV4/UV4.exe" #home 
UV="/d/software_config/keil_soft/keil_app/UV4/UV4.exe" #company
PROJ="project/MDK_V5/18_AT32F435VGT7_Decode.uvprojx"

BUILD_LOG="$PWD/build_log.txt"
FLASH_LOG="$PWD/flash_log.txt"

# ----- 存在性检查 -----
if [ ! -f "$UV" ]; then
    echo "Error: UV4.exe not found: $UV"
    exit 1
fi
if [ ! -f "$PROJ" ]; then
    echo "Error: Project file not found: $PROJ"
    echo "Please run this script from the project root."
    exit 1
fi

# ----- 功能函数 -----
do_build() {
    echo "=== Building project ==="
    "$UV" -j0 -r "$PROJ" -o "$BUILD_LOG"
    local result=$?
    if [ -f "$BUILD_LOG" ]; then
        cat "$BUILD_LOG"
    else
        echo "(No build log generated)"
    fi
    if [ $result -ne 0 ]; then
        echo "Build failed! Log saved at: $BUILD_LOG"
        exit 1
    fi
    echo "Build succeeded."
    # rm -f "$BUILD_LOG"
}

do_flash() {
    echo "=== Programming Flash ==="
    "$UV" -f "$PROJ" -o "$FLASH_LOG"
    local result=$?
    if [ -f "$FLASH_LOG" ]; then
        cat "$FLASH_LOG"
    else
        echo "(No flash log generated)"
    fi
    if [ $result -ne 0 ]; then
        echo "Flash failed! Log saved at: $FLASH_LOG"
        exit 1
    fi
    echo "Flash done."
    # rm -f "$FLASH_LOG"
}

# ----- 主逻辑 -----
if [ $# -eq 0 ]; then
    do_build
    do_flash
else
    case "$1" in
        -b) do_build ;;
        -f) do_flash ;;
        -a) do_build; do_flash ;;
        *)
            echo "Usage: $0 [-b | -f | -a]"
            echo "  (no args)  Build and flash"
            echo "  -b         Build only"
            echo "  -f         Flash only"
            echo "  -a         Build and flash"
            exit 1
            ;;
    esac
fi

# read -p "Press Enter to exit..." dummy