#include <algorithm>

#include "ErrorSystem.h"


ErrorSystem& ErrorSystem::instance()
{
    static ErrorSystem instance;
    return instance;
}

int ErrorSystem::registerError(const QString& message)
{
    QMutexLocker locker(&mutex_);
    errorTypes_.push_back(message);
    return errorTypes_.size()-1;
}

void ErrorSystem::clearHistory()
{
    QMutexLocker locker(&mutex_);
    loggedErrors_.clear();
}

void ErrorSystem::logError(int err, const QStringList& params)
{
    QMutexLocker locker(&mutex_);
    if (errorTypes_.size() > err)
        loggedErrors_.emplace_back(err, QDateTime::currentDateTime(), params);
}

QString ErrorSystem::formatError(const ErrorInfo& info) const
{    
    QString formatted = errorTypes_[info.error_];
    for (const QString& param : info.params_)
        formatted = formatted.arg(param);

    return QString("[%1] %2")
        .arg(info.timestamp_.toString("yyyy-MM-dd hh:mm:ss"))
        .arg(formatted);
}

QStringList ErrorSystem::getErrors(int index, int count) const
{
    QMutexLocker locker(&mutex_);

    QStringList errors;

    if (loggedErrors_.isEmpty())
        return errors;

    const int size = loggedErrors_.size();

    // Gestion de l'index négatif : -1 = dernier, -2 = avant-dernier, etc.
    if (index < 0)
        index += size;

    // Clamp aux bornes
    index = std::clamp(index, 0, size - 1);
    
    // Nombre maximum de messages à retourner
    if (count <= 0)
        count = size - index;

    const int end = std::min(index + count, size);
    const ErrorInfo* errorsList = loggedErrors_.data();
    for (int i = index; i < end; ++i)
        errors.append(formatError(errorsList[i]));

    return errors;
}
