#include <QMimeData>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QBuffer>
#include <QUrl>

#include "Clipboard.h"


void Clipboard::setText(const QString& text)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
}

QString Clipboard::getText() const
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    return clipboard->text();
}

bool Clipboard::hasUrls() const
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    return mimeData && mimeData->hasUrls();
}

QStringList Clipboard::getUrls() const
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    if (!mimeData || !mimeData->hasUrls())
        return QStringList();
    
    QStringList urls;
    for (const QUrl& url : mimeData->urls())
    {
        if (url.isLocalFile())
        {
            urls.append(url.toLocalFile());
        }
    }
    return urls;
}

bool Clipboard::hasImage() const
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();
    return mimeData && mimeData->hasImage();
}

QString Clipboard::getImageAsBase64() const
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    QImage image = clipboard->image();
    if (image.isNull())
        return QString();
    
    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");  // Convertir en PNG
    
    return QString("data:image/png;base64,%1").arg(QString::fromLatin1(byteArray.toBase64()));
}

QString Clipboard::fileToBase64(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return QString();
    
    QByteArray fileData = file.readAll();
    file.close();
    
    // Détecter le type MIME à partir de l'extension
    QString mimeType = "image/png";
    QString extension = QFileInfo(filePath).suffix().toLower();
    if (extension == "jpg" || extension == "jpeg")
        mimeType = "image/jpeg";
    else if (extension == "gif")
        mimeType = "image/gif";
    else if (extension == "webp")
        mimeType = "image/webp";
    
    return QString("data:%1;base64,%2")
        .arg(mimeType)
        .arg(QString::fromLatin1(fileData.toBase64()));
}
