#include "../Common/ErrorSystem.h"

#include "ErrorMessenger.h"

ErrorMessenger::ErrorMessenger(QObject* parent) :
    QObject(parent)
{
}

void ErrorMessenger::log(int err, const QStringList& params)
{
    ErrorSystem::instance().log(err, params);
    QStringList list = ErrorSystem::instance().getErrors(-1, 1);
    emit errorPopped(list.last());
}

QVariantList ErrorMessenger::get(int count) const
{
    QVariantList errors;

    QStringList list = ErrorSystem::instance().getErrors(0, 0);
    if (!list.size())
        return errors;

    if (count < 0)
        count = list.size();

    int index = list.size();
    while (--index >= 0 && count-- > 0)
        errors.append(QVariant().fromValue<QString>(list[index]));
     
    return errors;
}