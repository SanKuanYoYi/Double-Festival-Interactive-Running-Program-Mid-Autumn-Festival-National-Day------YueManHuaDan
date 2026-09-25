#ifndef SOUNDFX_H
#define SOUNDFX_H

/*
 * soundfx.h
 * 可选音效：默认静音（无需 QtMultimedia 模块）。
 *
 * 打开方式：
 *   1) YueManHuaDan.pro 里取消注释  DEFINES += YMH_SOUND  与  QT += multimedia
 *   2) 把 click.wav / catch.wav / chime.wav 放进 sounds/ 并在 resources.qrc 注册
 */

class SoundFx
{
public:
    static void playClick();   // UI 点击
    static void playCatch();   // 接住月饼
    static void playChime();   // 投影 / 粒子礼成
};

#endif // SOUNDFX_H
