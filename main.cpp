/*
 * main.cpp
 * 《月满华诞，码上团圆》入口。
 *
 * 运行方式：
 *   YueManHuaDan.exe                  正常交互模式
 *   YueManHuaDan.exe --shots <目录>    演示截图模式：自动走完「启动页 → 主场景 →
 *                                     投影 → 代码粒子 → 小游戏 → 结尾」，把每一幕
 *                                     导出为 PNG（用于 README / 答辩展示），完成后自动退出。
 */

#include "theme.h"

#include "mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QImage>
#include <QStringList>
#include <QTimer>
#include <QtGlobal>

namespace {

void saveShot(QWidget *widget, const QString &dir, const QString &fileName)
{
    const QImage img = widget->grab().toImage();
    const QString path = dir + QLatin1Char('/') + fileName;
    if (img.save(path))
        qWarning("[shots] wrote %s (%dx%d)", qPrintable(path), img.width(), img.height());
    else
        qWarning("[shots] FAILED to write %s", qPrintable(path));
}

} // namespace

int main(int argc, char *argv[])
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);
#endif

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("YueManHuaDan"));
    QApplication::setApplicationDisplayName(QStringLiteral("月满华诞 · 码上团圆"));
    QApplication::setApplicationVersion(QStringLiteral("2.0.0"));
    QApplication::setOrganizationName(QStringLiteral("河南师范大学软件学院"));
    app.setFont(Theme::font(14));

    // 解析 --shots <目录>
    QString shotsDir;
    const QStringList args = QApplication::arguments();
    for (int i = 1; i < args.size() - 1; ++i) {
        if (args.at(i) == QStringLiteral("--shots")) {
            shotsDir = args.at(i + 1);
            break;
        }
    }
    const bool demoMode = !shotsDir.isEmpty();
    if (demoMode) {
        QDir().mkpath(shotsDir);
        qWarning("[shots] demo mode -> %s", qPrintable(shotsDir));
    }

    MainWindow window;
    window.resize(Theme::kSceneW, Theme::kSceneH);
    window.show();

    if (demoMode) {
        // 依次推进各个阶段，每到一个关键节点抓一张图
        const struct ScriptItem
        {
            int delay;
            void (*action)(MainWindow *);
            const char *shot;
        } script[] = {
            {  500, nullptr,                                                      "01_start.png" },
            {  700, +[](MainWindow *w) { w->enterScene(); },                      nullptr        },
            { 2000, nullptr,                                                      "02_night_scene.png" },
            {  300, +[](MainWindow *w) { w->scene()->backdrop()->trigger(
                                             NightBackdrop::HitMooncake); },      nullptr        },
            { 1800, nullptr,                                                      "03_projection_anyang.png" },
            {  300, +[](MainWindow *w) { w->scene()->backdrop()->showProjection(
                                             NightBackdrop::OracleGuo); },        nullptr        },
            { 1800, nullptr,                                                      "04_projection_oracle.png" },
            {  300, +[](MainWindow *w) { w->scene()->backdrop()->clearProjection(); }, nullptr     },
            {  900, +[](MainWindow *w) { w->scene()->backdrop()->trigger(
                                             NightBackdrop::HitLaptop); },        nullptr        },
            { 1400, nullptr,                                                      "05_code_particles.png" },
            { 2200, nullptr,                                                      "05b_code_shape.png" },
            {  300, +[](MainWindow *w) { w->enterGame(); },                       nullptr        },
            { 1200, nullptr,                                                      "06_catch_game.png" },
            {  300, +[](MainWindow *w) { w->game()->demoForcePhase(1); },         nullptr        },
            {  900, nullptr,                                                      "07_game_77.png" },
            {  300, +[](MainWindow *w) { w->game()->demoForcePhase(2); },         nullptr        },
            {  900, nullptr,                                                      "08_game_guo.png" },
            {  300, +[](MainWindow *w) { w->showEnding(120, QStringList()
                                             << QStringLiteral("团圆")
                                             << QStringLiteral("昌盛")
                                             << QStringLiteral("华诞")
                                             << QStringLiteral("厚德")
                                             << QStringLiteral("博学")
                                             << QStringLiteral("至善")); },      nullptr        },
            { 2600, nullptr,                                                      "09_ending.png" },
            {  400, nullptr,                                                      nullptr        },
        };

        int elapsed = 0;
        for (const auto &item : script) {
            elapsed += item.delay;
            QTimer::singleShot(elapsed, &window, [&window, &shotsDir, item]() {
                if (item.action)
                    item.action(&window);
                if (item.shot)
                    saveShot(&window, shotsDir, QString::fromUtf8(item.shot));
            });
        }
        QTimer::singleShot(elapsed + 200, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
