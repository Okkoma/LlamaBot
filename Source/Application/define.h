#pragma once

#include <QtCore>

#if defined (Q_OS_ANDROID) || defined (Q_OS_IOS)
    #define MOBILE
#endif

template <typename EnumObj> int stringToEnumValue(const char* enumName, const QString& str)
{
    const QMetaObject& metaObj = EnumObj::staticMetaObject;
    return static_cast<int>(metaObj.enumerator(metaObj.indexOfEnumerator(enumName)).keyToValue(str.toUtf8().constData()));
}

template <typename EnumObj> QString enumValueToString(const char* enumName, int value)
{
    const QMetaObject& metaObj = EnumObj::staticMetaObject;
    return QString(metaObj.enumerator(metaObj.indexOfEnumerator(enumName)).valueToKey(value));
}

static const QString NULL_QSTRING = QString();