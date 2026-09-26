/****************************************************************************
** Meta object code from reading C++ file 'nightscene.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../nightscene.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'nightscene.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_NightBackdrop_t {
    QByteArrayData data[19];
    char stringdata0[186];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NightBackdrop_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NightBackdrop_t qt_meta_stringdata_NightBackdrop = {
    {
QT_MOC_LITERAL(0, 0, 13), // "NightBackdrop"
QT_MOC_LITERAL(1, 14, 11), // "moonChanged"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 11), // "requestGame"
QT_MOC_LITERAL(4, 39, 11), // "hintChanged"
QT_MOC_LITERAL(5, 51, 4), // "text"
QT_MOC_LITERAL(6, 56, 7), // "moonPos"
QT_MOC_LITERAL(7, 64, 8), // "moonGlow"
QT_MOC_LITERAL(8, 73, 7), // "HitRole"
QT_MOC_LITERAL(9, 81, 7), // "HitNone"
QT_MOC_LITERAL(10, 89, 7), // "HitMoon"
QT_MOC_LITERAL(11, 97, 11), // "HitMooncake"
QT_MOC_LITERAL(12, 109, 9), // "HitLaptop"
QT_MOC_LITERAL(13, 119, 8), // "HitStart"
QT_MOC_LITERAL(14, 128, 10), // "Projection"
QT_MOC_LITERAL(15, 139, 12), // "NoProjection"
QT_MOC_LITERAL(16, 152, 10), // "AnyangHome"
QT_MOC_LITERAL(17, 163, 9), // "OracleGuo"
QT_MOC_LITERAL(18, 173, 12) // "CampusMemory"

    },
    "NightBackdrop\0moonChanged\0\0requestGame\0"
    "hintChanged\0text\0moonPos\0moonGlow\0"
    "HitRole\0HitNone\0HitMoon\0HitMooncake\0"
    "HitLaptop\0HitStart\0Projection\0"
    "NoProjection\0AnyangHome\0OracleGuo\0"
    "CampusMemory"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NightBackdrop[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       2,   34, // properties
       2,   42, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x06 /* Public */,
       3,    0,   30,    2, 0x06 /* Public */,
       4,    1,   31,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,

 // properties: name, type, flags
       6, QMetaType::QPointF, 0x00495103,
       7, QMetaType::QReal, 0x00495103,

 // properties: notify_signal_id
       0,
       0,

 // enums: name, alias, flags, count, data
       8,    8, 0x0,    5,   52,
      14,   14, 0x0,    4,   62,

 // enum data: key, value
       9, uint(NightBackdrop::HitNone),
      10, uint(NightBackdrop::HitMoon),
      11, uint(NightBackdrop::HitMooncake),
      12, uint(NightBackdrop::HitLaptop),
      13, uint(NightBackdrop::HitStart),
      15, uint(NightBackdrop::NoProjection),
      16, uint(NightBackdrop::AnyangHome),
      17, uint(NightBackdrop::OracleGuo),
      18, uint(NightBackdrop::CampusMemory),

       0        // eod
};

void NightBackdrop::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NightBackdrop *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->moonChanged(); break;
        case 1: _t->requestGame(); break;
        case 2: _t->hintChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NightBackdrop::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NightBackdrop::moonChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NightBackdrop::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NightBackdrop::requestGame)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NightBackdrop::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NightBackdrop::hintChanged)) {
                *result = 2;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<NightBackdrop *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QPointF*>(_v) = _t->moonPos(); break;
        case 1: *reinterpret_cast< qreal*>(_v) = _t->moonGlow(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<NightBackdrop *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setMoonPos(*reinterpret_cast< QPointF*>(_v)); break;
        case 1: _t->setMoonGlow(*reinterpret_cast< qreal*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject NightBackdrop::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsObject::staticMetaObject>(),
    qt_meta_stringdata_NightBackdrop.data,
    qt_meta_data_NightBackdrop,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NightBackdrop::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NightBackdrop::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NightBackdrop.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsObject::qt_metacast(_clname);
}

int NightBackdrop::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 2;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void NightBackdrop::moonChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NightBackdrop::requestGame()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NightBackdrop::hintChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
struct qt_meta_stringdata_NightScene_t {
    QByteArrayData data[5];
    char stringdata0[44];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NightScene_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NightScene_t qt_meta_stringdata_NightScene = {
    {
QT_MOC_LITERAL(0, 0, 10), // "NightScene"
QT_MOC_LITERAL(1, 11, 11), // "requestGame"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 12), // "requestStart"
QT_MOC_LITERAL(4, 37, 6) // "onTick"

    },
    "NightScene\0requestGame\0\0requestStart\0"
    "onTick"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NightScene[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   29,    2, 0x06 /* Public */,
       3,    0,   30,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    0,   31,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,

       0        // eod
};

void NightScene::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NightScene *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->requestGame(); break;
        case 1: _t->requestStart(); break;
        case 2: _t->onTick(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NightScene::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NightScene::requestGame)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NightScene::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NightScene::requestStart)) {
                *result = 1;
                return;
            }
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject NightScene::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsScene::staticMetaObject>(),
    qt_meta_stringdata_NightScene.data,
    qt_meta_data_NightScene,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NightScene::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NightScene::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NightScene.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsScene::qt_metacast(_clname);
}

int NightScene::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsScene::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void NightScene::requestGame()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NightScene::requestStart()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
struct qt_meta_stringdata_NightSceneView_t {
    QByteArrayData data[1];
    char stringdata0[15];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NightSceneView_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NightSceneView_t qt_meta_stringdata_NightSceneView = {
    {
QT_MOC_LITERAL(0, 0, 14) // "NightSceneView"

    },
    "NightSceneView"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NightSceneView[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void NightSceneView::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject NightSceneView::staticMetaObject = { {
    QMetaObject::SuperData::link<QGraphicsView::staticMetaObject>(),
    qt_meta_stringdata_NightSceneView.data,
    qt_meta_data_NightSceneView,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NightSceneView::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NightSceneView::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NightSceneView.stringdata0))
        return static_cast<void*>(this);
    return QGraphicsView::qt_metacast(_clname);
}

int NightSceneView::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsView::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
