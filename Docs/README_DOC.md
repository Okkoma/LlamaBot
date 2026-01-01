/**
 * @mainpage Documentation LlamaBot
 * 
 * @tableofcontents
 * 
 * # LlamaBot - Application de Chat LLM basée sur Qt
 * 
 * **Version 1.0** - Documentation générée automatiquement
 * 
 * LlamaBot est une application moderne de chat LLM (Large Language Model) construite avec Qt et C++. 
 * Elle permet aux utilisateurs d'interagir avec différents modèles de langage localement ou via des APIs distantes.
 * 
 * ## Fonctionnalités principales
 * 
 * - **Multi-backend LLM** : Support pour Llama.cpp (local) et Ollama (serveur)
 * - **Interface utilisateur moderne** : Interface QML réactive et personnalisable
 * - **Gestion des chats** : Création, suppression et organisation des conversations
 * - **RAG (Retrieval-Augmented Generation)** : Augmentation des réponses avec des documents personnels
 * - **Thèmes personnalisables** : Mode sombre/clair et personnalisation des couleurs
 * - **Streaming en temps réel** : Réponses générées progressivement
 * - **Gestion des modèles** : Téléchargement et configuration des modèles LLM
 * 
 * ## Architecture Logicielle
 * 
 * LlamaBot suit une architecture MVC (Modèle-Vue-Contrôleur) avec les composants principaux suivants :
 * 
 * ### Couche Application
 * - **Application** : Point d'entrée principal
 * - **ChatController** : Gestion centrale des chats et conversations
 * - **ThemeManager** : Gestion des thèmes et de l'apparence
 * 
 * ### Couche Services LLM
 * - **LLMServices** : Gestion unifiée des services LLM
 * - **LlamaCppService** : Intégration avec Llama.cpp pour l'exécution locale
 * - **OllamaService** : Communication avec le serveur Ollama
 * 
 * ### Couche Données
 * - **Chat** / **ChatImpl** : Modèles de données pour les conversations
 * - **RAGService** : Recherche et récupération de contexte
 * - **VectorStore** : Stockage vectoriel pour les embeddings
 * 
 * ### Couche Interface
 * - **QML Components** : Interface utilisateur réactive
 * - **API Selector** : Sélection des APIs LLM
 * - **Model Selector** : Gestion des modèles disponibles
 * 
 * ## Diagramme d'Architecture
 * 
 * @dot
 * digraph architecture {
 *     rankdir=LR;
 *     node [shape=box, style=rounded, fontsize=12];
 *     edge [fontsize=10];
 * 
 *     // Couches
 *     subgraph cluster_application {
 *         label="Couche Application";
 *         color=lightblue;
 *         style=filled;
 *         bgcolor="#e6f3ff";
 * 
 *         Application;
 *         ChatController;
 *         ThemeManager;
 *     }
 * 
 *     subgraph cluster_services {
 *         label="Couche Services LLM";
 *         color=lightgreen;
 *         style=filled;
 *         bgcolor="#e6ffe6";
 * 
 *         LLMServices;
 *         LlamaCppService;
 *         OllamaService;
 *     }
 * 
 *     subgraph cluster_data {
 *         label="Couche Données";
 *         color=lightyellow;
 *         style=filled;
 *         bgcolor="#ffffe6";
 * 
 *         Chat;
 *         RAGService;
 *         VectorStore;
 *     }
 * 
 *     subgraph cluster_ui {
 *         label="Couche Interface";
 *         color=lightpink;
 *         style=filled;
 *         bgcolor="#ffe6e6";
 * 
 *         QML;
 *         APISelector;
 *         ModelSelector;
 *     }
 * 
 *     // Connexions
 *     Application -> ChatController;
 *     ChatController -> LLMServices;
 *     ChatController -> Chat;
 *     ChatController -> RAGService;
 *     ChatController -> ThemeManager;
 * 
 *     LLMServices -> LlamaCppService;
 *     LLMServices -> OllamaService;
 * 
 *     Chat -> ChatImpl [label="implémente"];
 * 
 *     ChatController -> QML [label="contrôle"];
 *     QML -> APISelector;
 *     QML -> ModelSelector;
 * 
 *     RAGService -> VectorStore;
 * }
 * @enddot
 * 
 * ## Guide de Démarrage Rapide
 * 
 * ### Prérequis
 * - Qt 6.x ou supérieur
 * - C++17 ou supérieur
 * - Doxygen (pour la documentation)
 * - Graphviz (pour les diagrammes)
 * 
 * ### Installation
 * ```bash
 * # Clonez le dépôt
 * git clone https://github.com/votre-repo/llamabot.git
 * cd llamabot
 * 
 * # Configurez et construisez
 * mkdir build && cd build
 * cmake ..
 * cmake --build . --config Release
 * 
 * # Exécutez l'application
 * ./LlamaBot
 * ```
 * 
 * ### Configuration
 * La configuration se fait via le fichier `config.json` ou l'interface utilisateur :
 * - Sélectionnez l'API LLM (Llama.cpp ou Ollama)
 * - Choisissez un modèle disponible
 * - Configurez les paramètres de génération
 * - Activez/désactivez le RAG selon vos besoins
 * 
 * ## Exemples d'Utilisation
 * 
 * ### Créer un nouveau chat
 * ```cpp
 * // Depuis le contrôleur de chat
 * ChatController* controller = new ChatController(llmServices);
 * controller->createChat();
 * controller->setModel("llama-2-7b-chat.gguf");
 * controller->sendMessage("Bonjour, comment ça va ?");
 * ```
 * 
 * ### Utiliser le RAG
 * ```cpp
 * // Charger des documents et les utiliser pour le contexte
 * RAGService* rag = controller->ragService();
 * rag->ingestFile("/chemin/vers/mon/document.pdf");
 * rag->setRagEnabled(true);
 * 
 * controller->sendMessage("Qu'est-ce que ce document dit sur les LLM ?");
 * ```
 * 
 * ### Changer de thème
 * ```cpp
 * // Changer l'apparence de l'application
 * ThemeManager* themeManager = new ThemeManager();
 * themeManager->setDarkMode(true);
 * themeManager->setTheme("ModernDark");
 * ```
 * 
 * ### Configuration avancée
 * ```cpp
 * // Configuration des paramètres LLM
 * LlamaCppService* llamaService = llmServices->get(LlamaCpp);
 * llamaService->setDefaultGpuLayers(32);  // Utiliser 32 couches GPU
 * llamaService->setDefaultContextSize(4096); // Taille de contexte
 * llamaService->setDefaultUseGpu(true); // Activer le GPU
 * ```
 * 
 * ## Documentation Supplémentaire
 * 
 * ### Diagrammes de Classes
 * Consultez les diagrammes de classes automatiquement générés pour comprendre les relations entre les composants :
 * - [Diagramme de la classe Application](classes.html)
 * - [Diagramme des services LLM](classes.html)
 * - [Hiérarchie des classes](hierarchy.html)
 * 
 * ### API Complète
 * - [Liste des classes](annotated.html)
 * - [Index des membres](functions.html)
 * - [Fichiers source](files.html)
 * 
 * ### Ressources Externes
 * - [Documentation Qt](https://doc.qt.io/)
 * - [Documentation Llama.cpp](https://github.com/ggerganov/llama.cpp)
 * - [Documentation Ollama](https://ollama.ai/)
 * 
 * ## Contribution
 * 
 * Nous accueillons les contributions ! Voici comment participer :
 * 
 * ### Signaler un bug
 * 1. Vérifiez que le bug n'est pas déjà signalé
 * 2. Ouvrez une issue avec une description claire
 * 3. Incluez les étapes pour reproduire le bug
 * 4. Ajoutez des captures d'écran si nécessaire
 * 
 * ### Proposer une fonctionnalité
 * 1. Ouvrez une issue décrivant la fonctionnalité
 * 2. Expliquez le cas d'utilisation
 * 3. Proposez une implémentation si possible
 * 
 * ### Soumettre du code
 * 1. Forkez le dépôt
 * 2. Créez une branche pour votre fonctionnalité (`git checkout -b feature/nouvelle-fonctionnalite`)
 * 3. Commitez vos changements (`git commit -m 'Ajout de nouvelle fonctionnalité'`)
 * 4. Poussez vers la branche (`git push origin feature/nouvelle-fonctionnalite`)
 * 5. Ouvrez une Pull Request
 * 
 * ### Style de Code
 * - Suivez les conventions C++ modernes
 * - Utilisez les commentaires Doxygen pour la documentation
 * - Respectez le formatage existant
 * - Écrivez des tests pour les nouvelles fonctionnalités
 * 
 * ## Licence
 * 
 * Ce projet est sous licence MIT. Consultez le fichier [LICENSE](LICENSE) pour plus de détails.
 * 
 * © 2025 LlamaBot Contributors
 * 
 */