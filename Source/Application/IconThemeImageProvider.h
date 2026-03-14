#pragma once

#include <QQuickImageProvider>
#include <QIcon>
#include <QPixmap>
#include <QDebug>

class IconThemeImageProvider : public QQuickImageProvider
{
public:
    IconThemeImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}

    QPixmap requestPixmap(const QString &id, QSize* size, const QSize& requestedSize) override
    {
        // 'id' correspond à ce qui suit "image://icon/" (ex: "text-plain")
        // Si l'icône n'existe pas dans le thème, on peut charger une icône par défaut
        QIcon icon = QIcon::fromTheme(id);
        if (icon.isNull())        
            icon = QIcon::fromTheme("unknown");

        // Déterminer la taille à retourner
        QSize actualSize = requestedSize.isValid() ? requestedSize : QSize(64, 64);
        if (size)
            *size = actualSize;

        return icon.pixmap(actualSize);
    }
};
