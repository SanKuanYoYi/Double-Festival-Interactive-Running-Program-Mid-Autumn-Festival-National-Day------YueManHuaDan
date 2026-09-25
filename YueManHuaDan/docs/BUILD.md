# 编译 / 运行 / 打包说明

> 适用：Windows（MinGW 或 MSVC）、Linux、macOS 均可，Qt 5.15 或 Qt 6.x。
> 本文档描述的环境为 **Windows + Qt 5.15.2 + MinGW 8.1（64 位）**，其它组合差异会单独标注。

## 0. 准备 Qt

- **Qt Creator 用户**：直接「打开项目」选中 `YueManHuaDan.pro`，选择套件后点 ▶ 运行即可。
- **命令行用户**：需要 Qt 的 `qmake`（安装目录下的 `<Qt>/<版本>/mingw81_64/bin/qmake.exe`）。
  确认版本：

  ```bash
  qmake -v
  ```

- **没有 Qt**：可以从 [Qt 官方下载页](https://download.qt.io/archive/qt/) 或用 `aqtinstall` 装一个
  （国内可用清华镜像）：

  ```bash
  pip install aqtinstall
  python -m aqt install-qt -O D:/QtEnv windows desktop 5.15.2 win64_mingw81
  python -m aqt install-tool -O D:/QtEnv windows desktop tools_mingw qt.tools.win64_mingw810
  ```

  装完后把 `D:/QtEnv/5.15.2/mingw81_64/bin` 和 `D:/QtEnv/Tools/mingw810_64/bin` 加进 PATH。

## 1. 外部构建（推荐，不污染源码树）

```bat
cd /d D:\YueManHuaDan
md build
cd build
qmake ..\YueManHuaDan.pro
mingw32-make -j4
```

MSVC 套件把最后一行换成 `nmake`（需先运行 `vcvarsall.bat`）。

`YueManHuaDan.pro` 里设置了：

```qmake
DESTDIR = $$OUT_PWD          # exe 落在构建目录根部
MOC_DIR = build/moc
OBJECTS_DIR = build/obj
RCC_DIR = build/rcc
```

所以外部构建时 exe 就是 `build\YueManHuaDan.exe`；如果直接在工程根目录跑 qmake，
exe 会落在工程根目录（`.gitignore` 里已经忽略了 `*.exe`）。

Qt 6 也可以用 CMake 构建，但本项目已按 qmake 组织，**推荐直接用 qmake 或 Qt Creator**。

## 2. 运行

```bat
build\YueManHuaDan.exe
```

自带的演示模式可以一口气跑完全部流程并逐幕截图（用于 README、答辩、海报）：

```bat
build\YueManHuaDan.exe --shots D:\shots
```

## 3. 打包（给别人双击就能跑）

### 方式 A：用工程自带的脚本（推荐）

```bash
# Git Bash / MSYS
./tools/deploy.sh D:/QtEnv/5.15.2/mingw81_64 D:/build-ymdh/YueManHuaDan.exe
```

```bat
:: cmd / 双击，参数可省略，脚本会自己找 Qt
tools\deploy.bat
```

产物在 `deploy/`，整个目录拷到任何 Windows 机器上都能直接双击运行，**目标机器不需要装 Qt**。

### 方式 B：手动 windeployqt

```bat
:: 1) 编译 Release
qmake ..\YueManHuaDan.pro CONFIG+=release
mingw32-make -j4

:: 2) 复制依赖到 deploy/ 目录
windeployqt --release --no-translations --dir ..\deploy YueManHuaDan.exe

:: 3) 把 deploy/ 整个压缩即可提交 / 分享
```

### 关于 windeployqt 的一个坑

部分 Qt 发行版（例如 aqtinstall 装的 5.15.2）自带的插件里带 `.gnu_debuglink` 段，
会被 `windeployqt` 误判成 **debug 版**，于是拒绝部署 platform 插件，报：

```
Unable to find the platform plugin.
```

此时打出来的目录里没有 `platforms\qwindows.dll`，双击 exe 会弹
「This application failed to start because no Qt platform plugin could be initialized」。

**`tools/deploy.sh` / `tools/deploy.bat` 已经内置了这个判断**：发现 platform 插件缺失时
自动改用直接复制的方式补齐下面这些文件，所以用脚本打包不会踩这个坑。

```text
YueManHuaDan.exe
Qt5Core.dll / Qt5Gui.dll / Qt5Widgets.dll
libgcc_s_seh-1.dll / libstdc++-6.dll / libwinpthread-1.dll
platforms\qwindows.dll
styles\qwindowsvistastyle.dll
imageformats\qjpeg.dll   （想用可选 .jpg 写实素材就必须有它）
```

想手动验证某个打包目录是否完整，最直接的办法是在 PATH 里**不含 Qt bin** 的终端里运行它。

## 4. 可选：重新生成素材

`images/` 里的内置图片由脚本程序化生成（纯 Python 标准库，不需要 Pillow）：

```bash
python tools/gen_assets.py
```

删掉图片也能跑：程序检测到资源缺失会自动改用矢量绘制。

## 5. 可选：换成真实照片

把 `school_night.jpg` / `anyang_projection.jpg` / `ending_card.png` 放进 `images/`，
在 `resources.qrc` 里取消对应注释后重新构建；或者不重新编译，直接把图片放进
exe 同级的 `images/` 目录。具体尺寸建议见 `images/README.txt`。

## 6. 可选：打开音效

1. 把 `click.wav` / `catch.wav` / `chime.wav` 放进 `sounds/`，并在 `resources.qrc` 里注册；
2. 取消 `YueManHuaDan.pro` 中下面两行的注释：

   ```qmake
   QT += multimedia
   DEFINES += YMH_SOUND
   ```

## 7. 常见问题

| 现象 | 原因与处理 |
| --- | --- |
| `qmake` 不是内部或外部命令 | 没加 Qt 的 bin 目录到 PATH；或直接用 Qt Creator 打开工程 |
| 双击 exe 提示找不到 Qt5Core.dll | 没打包依赖，用 `tools/deploy.sh` 或 `windeployqt` |
| 提示 `no Qt platform plugin could be initialized` | 打包目录缺 `platforms\qwindows.dll`，见第 3 节 windeployqt 的坑 |
| 中文显示为方框 | 系统缺少中文字体。Windows 上正常；Linux 下装 `fonts-noto-cjk` 之类即可。程序会优先用「微软雅黑 → 思源黑体 → Noto CJK」等中文字体 |
| 帧率偏低 | 小游戏窗口矩阵窗户较多；可在 `gamewidget.cpp` 里把窗户行列数调小 |
| 编译报 `toFillPolygon` 参数不匹配 | 这是 Qt6 才有的三参数重载，本工程已统一用 Qt5/Qt6 通用的写法 |
| 用 offscreen 平台跑出来中文是方框 | `offscreen` 插件没有中文字库，属正常现象，真机运行不受影响 |
