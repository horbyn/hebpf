#!/bin/bash
set -e

INSTALL_DIR="/usr/local/hebpf"

# 获取脚本所在目录与项目根目录
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ROOT_DIR=$(dirname "$SCRIPT_DIR")
BUILD_DIR="$ROOT_DIR/build"
RELEASE_DIR="$(pwd)/packages" # 最终压缩包的输出目录

mkdir -p "$RELEASE_DIR"

# 临时目录用于组装文件（默认在 /tmp）
TMP_DIR=$(mktemp -d)

# 设置退出时的清理逻辑：成功则删除，失败则保留临时目录
cleanup() {
  local exit_code=$?
  if [ $exit_code -eq 0 ]; then
    rm -rf "$TMP_DIR"
  else
    echo "⚠️ 打包过程中出现错误，退出码: $exit_code"
    echo "⚠️ 临时目录已保留以供排查: $TMP_DIR"
  fi
}
trap cleanup EXIT

check_file() {
  if [ ! -f "$1" ]; then
    echo "错误: 找不到文件 $1"
    echo "请确保在打包前已经成功执行了编译 (cmake --build build)"
    exit 1
  fi
}

# 辅助替换占位符
replace_placeholder() {
  local file=$1
  sed -i "s|@INSTALL_DIR@|${INSTALL_DIR}|g" "$file"
}

echo "===== 开始打包 Daemon ====="
CORE_NAME=hebpf
CORE_PKG_DIR="$TMP_DIR/$CORE_NAME"
mkdir -p "$CORE_PKG_DIR"

check_file "$BUILD_DIR/hebpf"
cp "$BUILD_DIR/hebpf" "$CORE_PKG_DIR/"

check_file "$SCRIPT_DIR/install.sh"
check_file "$SCRIPT_DIR/uninstall.sh"
check_file "$SCRIPT_DIR/hebpf.service"

cp "$SCRIPT_DIR/install.sh" "$CORE_PKG_DIR/"
cp "$SCRIPT_DIR/uninstall.sh" "$CORE_PKG_DIR/"
cp "$SCRIPT_DIR/hebpf.service" "$CORE_PKG_DIR/"

# 替换脚本和配置中的占位符
replace_placeholder "$CORE_PKG_DIR/install.sh"
replace_placeholder "$CORE_PKG_DIR/uninstall.sh"
replace_placeholder "$CORE_PKG_DIR/hebpf.service"

chmod +x "$CORE_PKG_DIR/install.sh"
chmod +x "$CORE_PKG_DIR/uninstall.sh"

tar -czf "$RELEASE_DIR/$CORE_NAME.tar.gz" -C "$TMP_DIR" $CORE_NAME
echo "✅ 成功打包: $RELEASE_DIR/$CORE_NAME.tar.gz"


echo "===== 开始打包 eBPF 程序 ====="

declare -a EBPF_COMPONENTS=(
  "src/services/top/libhebpf_top_sched_switch.so:hebpf-top"
  "src/services/lb/libhebpf_lb_xdp_generic.so:hebpf-lb"
  "src/services/runqlat/libhebpf_runqlat_sched.so:hebpf-runqlat"
  "src/services/acl/libhebpf_acl_xdp_generic.so:hebpf-acl-xdp"
  "src/services/acl/libhebpf_acl_tc.so:hebpf-acl-tc"
)

for COMP in "${EBPF_COMPONENTS[@]}"; do
  REL_SRC="${COMP%%:*}"
  PKG_NAME="${COMP##*:}"

  SRC_FILE="$BUILD_DIR/$REL_SRC"
  check_file "$SRC_FILE"

  TARGET_DIR="$TMP_DIR/$PKG_NAME"
  mkdir -p "$TARGET_DIR"

  cp "$SRC_FILE" "$TARGET_DIR/"

  tar -czf "$RELEASE_DIR/$PKG_NAME.tar.gz" -C "$TMP_DIR" "$PKG_NAME"
  echo "✅ 成功打包: $RELEASE_DIR/$PKG_NAME.tar.gz"
done

echo "===== 所有打包任务完成 ====="
ls -lh "$RELEASE_DIR"
