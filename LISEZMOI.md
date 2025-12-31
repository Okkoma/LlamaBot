# LlamaBot

Un assistant conversationnel local utilisant Qt 6 et llama.cpp.

## Vue d'ensemble

LlamaBot est un assistant conçu pour offrir une expérience de conversation fluide avec des modèles de langage locaux (LLM). Il intègre notamment un système RAG (Retrieval Augmented Generation) permettant d'interroger vos documents personnels en toute confidentialité.

## Dépendances

### Dépendances Externes (à installer)

Le projet nécessite certaines bibliothèques système qui ne sont pas incluses dans le dépôt :

*   **Qt 6** : Core, Widgets, Network, Qml, Quick, QuickWidgets, QuickControls2.

### Dépendances Incluses

*   **llama.cpp** : Le moteur d'inférence est inclus en tant que sous-projet dans `Source/ThirdParty/llama.cpp`.
*   **libpoppler** : inclus en tant que sous-projet dans `Source/ThirdParty/poppler`.

#### Dépendances pour libpoppler : libfontconfig-dev, libopenjp2.7-dev

## Compilation

### Méthode Standard

```bash
# Créer et entrer dans le dossier de build
mkdir build
cd build

# Configurer le projet
cmake ..

# Compiler
make -j$(nproc)
```

### Utilisation des Scripts de Build

Plusieurs scripts sont fournis à la racine pour automatiser la configuration et la compilation selon votre matériel :

*   `./build_native.sh` : Build standard avec CUDA, OpenCL et BLAS activés.
*   `./build_native_cpu_only.sh` : Build optimisé pour CPU uniquement.
*   `./build_native_gpu_cuda.sh` : Build spécifique pour NVIDIA (CUDA).
*   `./build_native_gpu_cl.sh` : Build spécifique pour OpenCL.

Pour Android, des scripts spécifiques sont également disponibles (`build_android_*.sh`).

### Options de compilation (Backends GPU)

Vous pouvez activer l'accélération GPU via les options CMake dans le fichier racine (par défaut, CUDA et BLAS sont activés si détectés) :

*   `-DGGML_CUDA=ON` : Support NVIDIA CUDA
*   `-DGGML_VULKAN=ON` : Support Vulkan
*   `-DGGML_OPENCL=ON` : Support OpenCL (AMD/Intel)
*   `-DGGML_BLAS=ON` : Accélération CPU via BLAS

## Fonctionnalités

*   **Interface Moderne** : Interface fluide développée en QML avec support des modes clair et sombre.
*   **Inférence Locale** : Support complet des modèles au format `.gguf` via llama.cpp.
*   **Intégration Ollama** : Possibilité de se connecter à un serveur Ollama local.
*   **Système RAG** : Indexation et recherche dans vos documents (PDF, TXT, MD) pour des réponses basées sur votre base de connaissance locale.
*   **Confidentialité** : Tout fonctionne localement, aucune donnée ne quitte votre machine.

## En cours de développement

*   Support des modèles multimodales.
*   Etendre le choix des modèles disponibles : HuggingFace, et url manuel, OCI (voir projet Ramalama).
*   Système de regroupement/classification des conversations.
*   Tests système (Qml Squish).

## Licence

Ce projet est sous licence **MIT**. Voir le fichier [LICENSE](LICENSE) pour plus de détails.
