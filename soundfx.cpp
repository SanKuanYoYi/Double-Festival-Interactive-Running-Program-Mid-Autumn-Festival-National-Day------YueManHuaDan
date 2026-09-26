#include "soundfx.h"

#ifdef YMH_SOUND
#  include <QSoundEffect>
#  include <QUrl>
#  include <QString>

namespace {
void playWav(const QString &resPath, qreal volume)
{
    static QSoundEffect effect;
    static QString current;
    if (current != resPath) {
        effect.setSource(QUrl(resPath));
        current = resPath;
    }
    effect.setVolume(volume);
    effect.play();
}
} // namespace
#endif

void SoundFx::playClick()
{
#ifdef YMH_SOUND
    playWav(QStringLiteral("qrc:/sounds/click.wav"), 0.45);
#endif
}

void SoundFx::playCatch()
{
#ifdef YMH_SOUND
    playWav(QStringLiteral("qrc:/sounds/catch.wav"), 0.55);
#endif
}

void SoundFx::playChime()
{
#ifdef YMH_SOUND
    playWav(QStringLiteral("qrc:/sounds/chime.wav"), 0.5);
#endif
}
