#!/bin/bash
cd "$(dirname "$0")"

# ============================================
# 可调参数（请根据实际情况修改）
# ============================================

#home 
#ARMCLANG_BIN="D:/soft/keil5/ARM/ARMCLANG/bin/armclang.exe"
#ARMCLANG_INC="D:/soft/keil5/ARM/ARMCLANG/include"
# company
ARMCLANG_BIN="D:/software_config/keil_soft/keil_app/ARM/ARMCLANG/bin/armclang.exe"
ARMCLANG_INC="D:/software_config/keil_soft/keil_app/ARM/ARMCLANG/include"
TARGET="arm-none-eabi"

# ============================================
# 脚本主体（自动下载 jq）
# ============================================
echo "Running uv2clangd.exe ..."
./uv2clangd.exe

if [ ! -f compile_commands.json ]; then
    echo "Error: compile_commands.json not found."
    exit 1
fi

echo "Inserting extra compile arguments ..."

# 自动安装 jq
if ! command -v jq &> /dev/null; then
    echo "jq not found, downloading ..."
    mkdir -p /usr/bin
    curl -L -o /usr/bin/jq.exe https://github.com/stedolan/jq/releases/latest/download/jq-win64.exe
    if [ $? -ne 0 ]; then
        echo "Download failed. Please install jq manually."
        exit 1
    fi
    chmod +x /usr/bin/jq.exe
fi

# 构建额外参数（仅必要项）
EXTRA_ARGS=(
    "$ARMCLANG_BIN"
    "-target" "$TARGET"
    "-I" "$ARMCLANG_INC"
)

# 转换为 JSON 数组（兼容旧版 jq）
EXTRA_JSON=$(printf '%s\n' "${EXTRA_ARGS[@]}" | jq -R . | jq -s .)

# 插入参数（防重复）
jq --argjson extra "$EXTRA_JSON" '
    map(if .arguments[0] == $extra[0] then . else .arguments = $extra + .arguments end)
' compile_commands.json > tmp.json && mv tmp.json compile_commands.json

mkdir -p .vscode
mv compile_commands.json .vscode/
echo "Done! Generated .vscode/compile_commands.json"