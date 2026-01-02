/**
 * @page examples Exemples d'Utilisation
 * 
 * @tableofcontents
 * 
 * # Exemples d'Utilisation de LlamaBot
 * 
 * Cette page contient des exemples complets d'utilisation de l'API LlamaBot.
 * 
 * ## Exemple 1: Création et gestion d'un chat simple
 * 
 * ```cpp
 * // Initialisation de l'application
 * int argc = 1;
 * char* argv[] = {(char*")LlamaBot"};
 * Application app(argc, argv);
 * 
 * // Création du contrôleur de chat
 * LLMServices* llmServices = new LLMServices(&app);
 * ChatController* controller = new ChatController(llmServices);
 * 
 * // Création d'un nouveau chat
 * controller->createChat();
 * controller->setName("Mon premier chat");
 * 
 * // Sélection d'un modèle LLM
 * controller->setAPI("llama.cpp");
 * controller->setModel("llama-2-7b-chat.gguf");
 * 
 * // Envoi d'un message
 * controller->sendMessage("Bonjour, comment ça va aujourd'hui ?");
 * 
 * // Récupération de la réponse
 * Chat* currentChat = controller->currentChat();
 * QStringList messages = currentChat->getMessages();
 * 
 * // Affichage des messages
 * for (const QString& message : messages) {
 *     qDebug() << "Message:" << message;
 * }
 * ```
 * 
 * ## Exemple 2: Utilisation du RAG avec des documents
 * 
 * ```cpp
 * // Initialisation du service RAG
 * RAGService* ragService = controller->ragService();
 * 
 * // Ingestion de documents
 * ragService->ingestFile("/chemin/vers/document1.pdf");
 * ragService->ingestFile("/chemin/vers/document2.txt");
 * 
 * // Activation du RAG
 * ragService->setRagEnabled(true);
 * 
 * // Envoi d'une question avec contexte RAG
 * controller->sendMessage("Quelle est la conclusion principale du document1 ?");
 * 
 * // Récupération du contexte utilisé
 * QString context = ragService->retrieveContext("conclusion document1");
 * qDebug() << "Contexte RAG:" << context;
 * ```
 * 
 * ## Exemple 3: Configuration avancée de Llama.cpp
 * 
 * ```cpp
 * // Récupération du service Llama.cpp
 * LlamaCppService* llamaService = 
 *     static_cast<LlamaCppService*>(llmServices->get("llama.cpp"));
 * 
 * // Configuration GPU
 * llamaService->setDefaultGpuLayers(32);  // Utiliser 32 couches GPU
 * llamaService->setDefaultUseGpu(true);   // Activer le GPU
 * 
 * // Configuration du contexte
 * llamaService->setDefaultContextSize(4096); // Taille de contexte en tokens
 * 
 * // Activation de la version threadée
 * llamaService->setUseThreadedVersion(true);
 * 
 * // Configuration pour un chat spécifique
 * Chat* chat = controller->currentChat();
 * llamaService->setModel(chat, "mistral-7b-instruct-v0.1.Q4_K_M.gguf");
 * ```
 * 
 * ## Exemple 4: Gestion des thèmes et de l'interface
 * 
 * ```cpp
 * // Initialisation du gestionnaire de thèmes
 * ThemeManager* themeManager = new ThemeManager();
 * 
 * // Configuration du thème
 * themeManager->setDarkMode(true);
 * themeManager->setTheme("ModernDark");
 * 
 * // Personnalisation des couleurs
 * QColor primaryColor = themeManager->color("primary");
 * QColor secondaryColor = themeManager->color("secondary");
 * 
 * // Liste des thèmes disponibles
 * QStringList themes = themeManager->availableThemes();
 * for (const QString& theme : themes) {
 *     qDebug() << "Thème disponible:" << theme;
 * }
 * 
 * // Changement de police
 * QFont customFont("Roboto", 12);
 * themeManager->setFont(customFont);
 * ```
 * 
 * ## Exemple 5: Gestion avancée des modèles Ollama
 * 
 * ```cpp
 * // Récupération du service Ollama
 * OllamaService* ollamaService = 
 *     static_cast<OllamaService*>(llmServices->get("ollama"));
 * 
 * // Configuration du serveur
 * ollamaService->setUrl("http://localhost:11434");
 * ollamaService->setApiVersion("v1");
 * 
 * // Démarrage du serveur si nécessaire
 * if (!ollamaService->isProcessStarted()) {
 *     ollamaService->start();
 * }
 * 
 * // Récupération des modèles disponibles
 * std::vector<LLMModel> models = ollamaService->getAvailableModels();
 * for (const LLMModel& model : models) {
 *     qDebug() << "Modèle:" << model.name 
 *              << "Taille:" << model.size 
 *              << "Modifié:" << model.modified;
 * }
 * 
 * // Utilisation d'un modèle spécifique
 * controller->setAPI("ollama");
 * controller->setModel("llama2:7b-chat");
 * ```
 * 
 * ## Exemple 6: Sérialisation et sauvegarde des chats
 * 
 * ```cpp
 * // Récupération du chat courant
 * Chat* chat = controller->currentChat();
 * 
 * // Sérialisation en JSON
 * QJsonObject chatJson = chat->toJson();
 * QJsonDocument doc(chatJson);
 * QString jsonString = doc.toJson();
 * 
 * // Sauvegarde dans un fichier
 * QFile file("chat_backup.json");
 * if (file.open(QIODevice::WriteOnly)) {
 *     file.write(jsonString.toUtf8());
 *     file.close();
 * }
 * 
 * // Chargement depuis un fichier
 * if (file.open(QIODevice::ReadOnly)) {
 *     QByteArray data = file.readAll();
 *     QJsonDocument loadedDoc = QJsonDocument::fromJson(data);
 *     QJsonObject loadedJson = loadedDoc.object();
 *     
 *     // Création d'un nouveau chat et chargement des données
 *     Chat* newChat = controller->createChat();
 *     newChat->fromJson(loadedJson);
 *     
 *     file.close();
 * }
 * ```
 * 
 * ## Exemple 7: Gestion des erreurs et des événements
 * 
 * ```cpp
 * // Connexion aux signaux du contrôleur de chat
 * QObject::connect(controller, &ChatController::chatContentUpdated, 
 *     [](Chat* chat) {
 *         qDebug() << "Chat mis à jour:" << chat->getName();
 *     });
 * 
 * QObject::connect(controller, &ChatController::processingStarted, 
 *     [](Chat* chat) {
 *         qDebug() << "Traitement commencé pour:" << chat->getName();
 *     });
 * 
 * QObject::connect(controller, &ChatController::processingFinished, 
 *     [](Chat* chat) {
 *         qDebug() << "Traitement terminé pour:" << chat->getName();
 *     });
 * 
 * // Gestion des erreurs RAG
 * QObject::connect(ragService, &RAGService::errorOccurred,
 *     [](const QString& error) {
 *         qWarning() << "Erreur RAG:" << error;
 *     });
 * 
 * // Gestion des erreurs de thème
 * QObject::connect(themeManager, &ThemeManager::styleNotAvailableWarning,
 *     [](const QString& styleName) {
 *         qWarning() << "Style non disponible:" << styleName;
 *     });
 * ```
 * 
 * ## Exemple 8: Utilisation avancée du streaming
 * 
 * ```cpp
 * // Connexion au signal de streaming
 * QObject::connect(chat, &Chat::streamUpdated,
 *     [](const QString& text) {
 *         qDebug() << "Token reçu:" << text;
 *         // Mettre à jour l'interface utilisateur en temps réel
 *     });
 * 
 * // Connexion au signal de fin de streaming
 * QObject::connect(chat, &Chat::streamFinishedSignal,
 *     []() {
 *         qDebug() << "Streaming terminé !";
 *     });
 * 
 * // Envoi d'un message avec streaming activé
 * controller->sendMessage("Raconte-moi une histoire longue et détaillée", true);
 * 
 * // Arrêt du streaming si nécessaire
 * controller->stopGeneration();
 * ```
 * 
 * ## Exemple 9: Intégration avec l'interface QML
 * 
 * ```qml
 * // Dans un fichier QML
 * import QtQuick 2.15
 * import LlamaBot 1.0
 * 
 * Item {
 *     width: 800
 *     height: 600
 * 
 *     // Accès au contrôleur de chat
 *     ChatController {
 *         id: chatController
 *         
 *         onChatListChanged: {
 *             console.log("Liste des chats changée")
 *         }
 *         
 *         onCurrentChatChanged: {
 *             console.log("Chat courant changé:", currentChat.name)
 *         }
 *     }
 * 
 *     // Bouton pour créer un nouveau chat
 *     Button {
 *         text: "Nouveau Chat"
 *         onClicked: {
 *             chatController.createChat()
 *         }
 *     }
 * 
 *     // Liste des chats
 *     ListView {
 *         model: chatController.chatList
 *         delegate: Text {
 *             text: modelData.name
 *         }
 *     }
 * 
 *     // Zone de message
 *     TextField {
 *         id: messageInput
 *         width: parent.width - 100
 *         height: 40
 *         
 *         onAccepted: {
 *             chatController.sendMessage(text)
 *             text = ""
 *         }
 *     }
 * 
 *     // Bouton d'envoi
 *     Button {
 *         text: "Envoyer"
 *         onClicked: {
 *             chatController.sendMessage(messageInput.text)
 *             messageInput.text = ""
 *         }
 *     }
 * }
 * ```
 * 
 * ## Exemple 10: Benchmarking et performances
 * 
 * ```cpp
 * // Mesure des performances de génération
 * QElapsedTimer timer;
 * timer.start();
 * 
 * controller->sendMessage("Génère un texte de 500 mots sur les LLM");
 * 
 * // Attente de la fin du traitement
 * while (controller->currentChat()->isProcessing()) {
 *     QApplication::processEvents();
 * }
 * 
 * qint64 elapsed = timer.elapsed();
 * qDebug() << "Temps de génération:" << elapsed << "ms";
 * 
 * // Mesure de l'utilisation des tokens
 * Chat* chat = controller->currentChat();
 * int tokensUsed = chat->getNumContextSizeUsed();
 * int tokensTotal = chat->getNumContextSize();
 * 
 * qDebug() << "Tokens utilisés:" << tokensUsed << "/" << tokensTotal;
 * qDebug() << "Utilisation:" << (tokensUsed * 100.0 / tokensTotal) << "%";
 * 
 * // Benchmarking GPU vs CPU
 * LlamaCppService* llamaService = 
 *     static_cast<LlamaCppService*>(llmServices->get("llama.cpp"));
 * 
 * // Test avec GPU
 * llamaService->setDefaultUseGpu(true);
 * timer.restart();
 * controller->sendMessage("Test GPU");
 * while (controller->currentChat()->isProcessing()) {
 *     QApplication::processEvents();
 * }
 * qint64 gpuTime = timer.elapsed();
 * 
 * // Test avec CPU
 * llamaService->setDefaultUseGpu(false);
 * timer.restart();
 * controller->sendMessage("Test CPU");
 * while (controller->currentChat()->isProcessing()) {
 *     QApplication::processEvents();
 * }
 * qint64 cpuTime = timer.elapsed();
 * 
 * qDebug() << "GPU:" << gpuTime << "ms, CPU:" << cpuTime << "ms";
 * qDebug() << "Accélération GPU:" << (cpuTime * 100.0 / gpuTime) << "%";
 * ```
 */