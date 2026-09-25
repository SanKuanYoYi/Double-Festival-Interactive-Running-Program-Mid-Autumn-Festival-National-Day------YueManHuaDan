#!/usr/bin/env bash
# =============================================================================
#  tools/deploy.sh —— 把 YueManHuaDan.exe 和它依赖的 Qt DLL 打成一个可直接
#  双击运行的文件夹（Git Bash / MSYS 下执行）。
#
#  用法：
#      tools/deploy.sh [QTDIR] [EXE] [OUTDIR]
#
#      QTDIR    Qt 套件目录（含 bin/qmake.exe 与 plugins/），
#               例如 D:/QtEnv/5.15.2/mingw81_64
#               省略时依次读 $QTDIR、常见安装路径自动探测
#      EXE      已编译出的可执行文件，默认 build/YueManHuaDan.exe
#      OUTDIR   输出目录，默认 deploy
#
#  说明：脚本先调用 windeployqt。部分 Qt 发行版的插件带 .gnu_debuglink 段，
#       会被 windeployqt 误判成 debug 版而拒绝部署 platform 插件
#       （报 "Unable to find the platform plugin"）。检测到这种情况时脚本会
#       自动退回「直接复制 DLL」的方式，保证打包结果一定能跑。
# =============================================================================

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJ_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

QT_DIR="${1:-${QTDIR:-}}"
EXE="${2:-$PROJ_DIR/build/YueManHuaDan.exe}"
OUTDIR="${3:-$PROJ_DIR/deploy}"

# ---- 1. 找 Qt ---------------------------------------------------------------
if [ -z "$QT_DIR" ]; then
    echo "[deploy] 未指定 QTDIR，尝试自动探测..."
    for root in "$PROJ_DIR/.qt-env" "$HOME/Qt" "C:/Qt" "D:/Qt" "C:/QtEnv" "D:/QtEnv"; do
        [ -d "$root" ] || continue
        for kit in "$root"/*/mingw81_64 "$root"/*/mingw_64 "$root"/*/win64_mingw81; do
            if [ -f "$kit/bin/qmake.exe" ]; then
                QT_DIR="$kit"
                break
            fi
        done
        [ -n "$QT_DIR" ] && break
    done
fi

if [ -z "$QT_DIR" ] || [ ! -f "$QT_DIR/bin/qmake.exe" ]; then
    echo "[deploy] 错误：没找到 Qt。请把 Qt 目录作为第一个参数传进来，例如"
    echo "         tools/deploy.sh D:/YueManHuaDan/.qt-env/5.15.2/mingw81_64"
    exit 1
fi
QT_DIR="${QT_DIR%/}"

if [ ! -f "$EXE" ]; then
    echo "[deploy] 错误：找不到可执行文件 $EXE，请先编译。"
    exit 1
fi

echo "[deploy] Qt     : $QT_DIR"
echo "[deploy] exe    : $EXE"
echo "[deploy] 输出到 : $OUTDIR"
echo

mkdir -p "$OUTDIR"
cp -f "$EXE" "$OUTDIR/"

# ---- 2. windeployqt ---------------------------------------------------------
if [ -x "$QT_DIR/bin/windeployqt.exe" ]; then
    echo "[deploy] 调用 windeployqt ..."
    PATH="$QT_DIR/bin:$PATH" "$QT_DIR/bin/windeployqt.exe" \
        --release --no-translations --no-system-d3d-compiler --no-opengl-sw \
        --compiler-runtime --dir "$(cygpath -w "$OUTDIR" 2>/dev/null || echo "$OUTDIR")" \
        "$(cygpath -w "$OUTDIR/YueManHuaDan.exe" 2>/dev/null || echo "$OUTDIR/YueManHuaDan.exe")" \
        || echo "[deploy] windeployqt 返回非 0，继续做兜底检查..."
else
    echo "[deploy] 未找到 windeployqt，直接使用兜底复制。"
fi

# ---- 3. 兜底：确保关键文件齐全 ----------------------------------------------
if [ ! -f "$OUTDIR/platforms/qwindows.dll" ]; then
    echo "[deploy] platform 插件缺失 —— windeployqt 把插件误判成 debug 版了，"
    echo "[deploy] 改用直接复制方式补齐。"
    mkdir -p "$OUTDIR/platforms" "$OUTDIR/styles" "$OUTDIR/imageformats"

    for f in Qt5Core.dll Qt5Gui.dll Qt5Widgets.dll \
             libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
        [ -f "$QT_DIR/bin/$f" ] && cp -f "$QT_DIR/bin/$f" "$OUTDIR/" || true
    done
    [ -f "$QT_DIR/plugins/platforms/qwindows.dll" ] && \
        cp -f "$QT_DIR/plugins/platforms/qwindows.dll" "$OUTDIR/platforms/" || true
    [ -f "$QT_DIR/plugins/styles/qwindowsvistastyle.dll" ] && \
        cp -f "$QT_DIR/plugins/styles/qwindowsvistastyle.dll" "$OUTDIR/styles/" || true
    # imageformats 里的 qjpeg 是可选 .jpg 素材能读出来的前提
    for p in qjpeg qico qgif qsvg qwebp qtiff; do
        [ -f "$QT_DIR/plugins/imageformats/$p.dll" ] && \
            cp -f "$QT_DIR/plugins/imageformats/$p.dll" "$OUTDIR/imageformats/" || true
    done
fi

# MinGW 运行时（windeployqt --compiler-runtime 通常会带，这里再保一次底）
for r in libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll; do
    [ -f "$OUTDIR/$r" ] || { [ -f "$QT_DIR/bin/$r" ] && cp -f "$QT_DIR/bin/$r" "$OUTDIR/"; }
done

# 把可选素材一起带上，方便用户直接在打包目录里替换照片
if [ -d "$PROJ_DIR/images" ] && [ ! -d "$OUTDIR/images" ]; then
    mkdir -p "$OUTDIR/images"
    cp -f "$PROJ_DIR"/images/*.jpg "$OUTDIR/images/" 2>/dev/null || true
    cp -f "$PROJ_DIR"/images/*.png "$OUTDIR/images/" 2>/dev/null || true
fi

# ---- 4. 结果确认 -------------------------------------------------------------
echo
if [ -f "$OUTDIR/platforms/qwindows.dll" ]; then
    echo "[deploy] 完成，可以直接运行：$OUTDIR/YueManHuaDan.exe"
    echo "[deploy] 整个目录拷到别的 Windows 机器上（未装 Qt 也行）即可运行。"
else
    echo "[deploy] 失败：仍然没有 platforms/qwindows.dll，请检查"
    echo "         $QT_DIR/plugins/platforms 是否存在。"
    exit 1
fi
