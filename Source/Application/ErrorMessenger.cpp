#include "ErrorSystem.h"

#include "ErrorMessenger.h"

ErrorMessenger::ErrorMessenger(QObject* parent) :
    QObject(parent)
{
    if (ErrorSystem::instancePtr())
        connect(ErrorSystem::instancePtr(), &ErrorSystem::errorAdded, this, &ErrorMessenger::notify);
    else
        qWarning() << "ErrorMessenger: no ErrorSystem";
}

void ErrorMessenger::log(int err, const QStringList& params)
{
    ErrorSystem::instance().log(err, params);
    notify(-1);
}

void ErrorMessenger::notify(int msgIndex)
{
    QStringList list = ErrorSystem::instance().getErrors(msgIndex, 1);
    qDebug() << "ErrorMessenger: notify: " << list.last();    
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