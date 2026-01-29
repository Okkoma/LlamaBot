#pragma once

#include <QApplication>
#include <QQmlEngine>
#include <QFont>

#include "ApplicationServices.h"

#include "ChatController.h"
#include "Clipboard.h"
#include "ModelStore.h"
#include "ThemeManager.h"

class QQmlApplicationEngine;

/**
 * @class Application
 * @brief Classe principale de l'application LlamaBot
 * 
 * Cette classe étend QApplication et sert de point d'entrée principal
 * pour l'application de chat LLM basée sur Qt.
 * 
 * Elle gère l'initialisation de tous les composants principaux:
 * - Moteur QML pour l'interface utilisateur
 * - Contrôleur de chat pour la gestion des conversations
 * - Services d'application pour les fonctionnalités transverses
 * - Gestion des thèmes et du presse-papiers
 */
class Application : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(ChatController* chatController READ chatController CONSTANT)
    Q_PROPERTY(ModelStore* modelStore READ modelStore CONSTANT)
    Q_PROPERTY(ThemeManager* themeManager READ themeManager CONSTANT)
    Q_PROPERTY(Clipboard* clipboard READ clipboard CONSTANT)

    QML_ELEMENT
    QML_UNCREATABLE("Application is a singleton provided by the application")

public:
    /**
     * @brief Constructeur de l'application
     * @param argc Nombre d'arguments de la ligne de commande
     * @param argv Tableau d'arguments de la ligne de commande
     * 
     * Initialise l'application Qt et tous ses composants.
     */
    explicit Application(int& argc, char** argv);
    
    /**
     * @brief Destructeur de l'application
     * 
     * Nettoie les ressources allouées par l'application.
     */
    ~Application() override;

    ChatController* chatController() const { return chatController_; }
    ModelStore* modelStore() const { return modelStore_; }
    ThemeManager* themeManager() const;
    Clipboard* clipboard() const { return clipboard_; }

private:
    QQmlApplicationEngine* qmlEngine_;    ///< Moteur QML pour le rendu de l'interface utilisateur
    ChatController* chatController_;      ///< Contrôleur pour la gestion des chats et conversations
    ModelStore* modelStore_;              ///< Gestionnaire des modèles
    ApplicationServices services_;        ///< Services d'application pour les fonctionnalités transverses
    Clipboard* clipboard_;                ///< Gestionnaire du presse-papiers système
};
