#!/bin/bash
set -e

if [ "$EUID" -ne 0 ]; then
  echo "错误: 请使用 root 权限运行 (sudo ./uninstall.sh)"
  exit 1
fi

INSTALL_DIR="@INSTALL_DIR@"

SERVICE_NAME="hebpf.service"
SERVICE_FILE="/etc/systemd/system/$SERVICE_NAME"

echo "正在检查并停止 hebpf 服务..."
if systemctl is-active --quiet "$SERVICE_NAME"; then
  systemctl stop "$SERVICE_NAME"
  echo "服务已停止。"
fi

echo "取消开机自启..."
if systemctl is-enabled --quiet "$SERVICE_NAME" 2>/dev/null; then
  systemctl disable "$SERVICE_NAME"
fi

echo "正在移除 systemd 配置文件..."
rm -f "$SERVICE_FILE"
systemctl daemon-reload

echo "正在删除安装目录 ($INSTALL_DIR)..."
rm -rf "$INSTALL_DIR"

echo "=========================================="
echo "✅ 卸载完成！"
echo "=========================================="
