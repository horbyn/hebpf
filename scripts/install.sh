#!/bin/bash
set -e

if [ "$EUID" -ne 0 ]; then
  echo "错误: 请使用 root 权限运行 (sudo ./install.sh)"
  exit 1
fi

INSTALL_DIR="@INSTALL_DIR@"

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

BIN_DIR="$INSTALL_DIR/bin"
CONF_DIR="$INSTALL_DIR/conf"
LOG_DIR="$INSTALL_DIR/logs"
SERVICE_NAME="hebpf.service"
SERVICE_DEST="/etc/systemd/system/$SERVICE_NAME"

echo "正在创建安装目录..."
mkdir -p "$BIN_DIR" "$CONF_DIR" "$LOG_DIR"

echo "正在安装二进制文件..."
cp "$SCRIPT_DIR/hebpf" "$BIN_DIR/"
chmod +x "$BIN_DIR/hebpf"

echo "正在生成默认配置文件..."
cd "$CONF_DIR"
"$BIN_DIR/hebpf" --get-conf

echo "正在安装 systemd 服务..."
cp "$SCRIPT_DIR/$SERVICE_NAME" "$SERVICE_DEST"
chmod 644 "$SERVICE_DEST"

echo "重新加载 systemd 并设置开机自启..."
systemctl daemon-reload
systemctl enable "$SERVICE_NAME"

echo "=========================================="
echo "✅ 安装完成！"
echo "启动服务: systemctl start hebpf"
echo "查看状态: systemctl status hebpf"
echo "=========================================="
