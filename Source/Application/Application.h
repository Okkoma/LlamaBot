#pragma once

#include <QApplication>
#include <QFont>

#include "ApplicationServices.h"

class QQmlApplicationEngine;
class ChatController;
class Clipboard;
class OllamaModelStoreDialog;
class ThemeManager;

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

private:
    QQmlApplicationEngine* qmlEngine_;    ///< Moteur QML pour le rendu de l'interface utilisateur
    ChatController* chatController_;      ///< Contrôleur pour la gestion des chats et conversations
    OllamaModelStoreDialog* modelStoreDialog_; ///< Dialogue pour la gestion des modèles Ollama
    ApplicationServices services_;        ///< Services d'application pour les fonctionnalités transverses
    ThemeManager* themeManager_;          ///< Gestionnaire des thèmes de l'application
    Clipboard* clipboard_;                ///< Gestionnaire du presse-papiers système
};
