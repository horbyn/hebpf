FROM ubuntu:jammy

# 禁止交互式时区设置参考：https://serverfault.com/a/1016972
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Shanghai

# x86_64 或 aarch64
ARG ARCH

# 设置语言
ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8
ENV LANGUAGE=en_US.UTF-8

RUN apt-get update && apt install -y --no-install-recommends ca-certificates && \
    cp /etc/apt/sources.list /etc/apt/sources.list.bak && \
    if [ "$ARCH" = "aarch64" ]; then \
        sed -i -e 's@//ports.ubuntu.com/\? @//ports.ubuntu.com/ubuntu-ports @g' \
            -e 's@//ports.ubuntu.com@//mirrors.ustc.edu.cn@g' \
            /etc/apt/sources.list; \
    elif [ "$ARCH" = "x86_64" ]; then \
        sed -i 's@//.*archive.ubuntu.com@//mirrors.ustc.edu.cn@g' /etc/apt/sources.list; \
        sed -i 's/security.ubuntu.com/mirrors.ustc.edu.cn/g' /etc/apt/sources.list; \
        sed -i 's/http:/https:/g' /etc/apt/sources.list; \
    else \
        echo "未知架构，无法设置 apt 代理"; \
        exit 1; \
    fi && \
    apt-get update && apt install -y --no-install-recommends \
        build-essential make gdb sudo vim wget git cmake gcovr valgrind \
        pkg-config clang libelf1 libelf-dev zlib1g-dev \
        systemd init plocate language-pack-en tree zsh && \
    rm -rf /var/lib/apt/lists/* && \
    apt-get clean && \
    # 中文
    sed -i -e 's/# zh_CN.UTF-8 UTF-8/zh_CN.UTF-8 UTF-8/' /etc/locale.gen && \
    dpkg-reconfigure --frontend=noninteractive locales && \
    # 安装 zsh
    git clone https://github.com/ohmyzsh/ohmyzsh.git /root/.oh-my-zsh && \
    cp /root/.oh-my-zsh/templates/zshrc.zsh-template /root/.zshrc && \
    git clone https://github.com/zsh-users/zsh-syntax-highlighting.git ~/.oh-my-zsh/plugins/zsh-syntax-highlighting && \
    git clone https://github.com/zsh-users/zsh-autosuggestions ~/.oh-my-zsh/plugins/zsh-autosuggestions && \
    chsh -s $(which zsh) && \
    sed -i 's/^plugins=(git)$/plugins=(git wd zsh-syntax-highlighting zsh-autosuggestions)/' ~/.zshrc && \
    # 创建调试版 gdb
    printf '#!/bin/bash\n\nsudo /usr/bin/gdb $@' > /usr/bin/gdb_sudo && \
    chmod +x /usr/bin/gdb_sudo && \
    # mac 全局忽略 .DS_Store 配置文件
    # ref to: https://orianna-zzo.github.io/sci-tech/2018-01/mac%E4%B8%ADgit%E5%BF%BD%E7%95%A5.ds_store%E6%96%87%E4%BB%B6/
    echo "# Mac OS specified" > ~/.gitignore_global && \
    echo "**/.DS_Store" > ~/.gitignore_global && \
    git config --global core.excludesfile ~/.gitignore_global

CMD ["/sbin/init"]
