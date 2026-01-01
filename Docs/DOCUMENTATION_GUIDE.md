# Guide de Documentation LlamaBot

Ce guide explique comment la documentation Doxygen est organisée et comment la générer.

## Structure de la Documentation

### Fichiers Principaux

1. **mainpage.dox** - Page principale de la documentation
   - Contient la description du projet
   - Diagramme d'architecture principal
   - Guide de démarrage rapide
   - Liens vers les autres sections

2. **architecture.dox** - Architecture détaillée
   - Description complète de l'architecture
   - Diagrammes de flux de données
   - Patterns de conception utilisés
   - Gestion des états et persistance

3. **examples.md** - Exemples d'utilisation
   - 10 exemples complets d'utilisation de l'API
   - Code prêt à l'emploi
   - Cas d'utilisation variés

4. **Doxyfile** - Configuration Doxygen
   - Configuration complète pour la génération
   - Support Markdown et diagrammes
   - Optimisation des formats de sortie

5. **DoxyLayout.xml** - Mise en page personnalisée
   - Organisation des onglets de navigation
   - Hiérarchie des éléments
   - Masquage des éléments non pertinents

### Fichiers de Code Documentés

Tous les fichiers dans `Source/Application/` avec des commentaires Doxygen :
- Application.h
- ChatController.h
- Chat.h
- ChatImpl.h
- LLMServices.h
- LlamaCppService.h
- OllamaService.h
- RAGService.h
- ThemeManager.h

## Génération de la Documentation

### Prérequis

1. **Doxygen** - Outil de génération de documentation
2. **Graphviz** - Pour générer les diagrammes (dot)

#### Installation

**Ubuntu/Debian:**
```bash
sudo apt-get install doxygen graphviz
```

**Fedora:**
```bash
sudo dnf install doxygen graphviz
```

**macOS (Homebrew):**
```bash
brew install doxygen graphviz
```

### Méthodes de Génération

#### Méthode 1: Utiliser le script bash (recommandé)
```bash
./generate_docs.sh
```

#### Méthode 2: Utiliser CMake
```bash
mkdir build && cd build
cmake ..
cmake --build . --target doc
```

#### Méthode 3: Utiliser Doxygen directement
```bash
doxygen Doxyfile
```

### Résultat

La documentation sera générée dans :
```
docs/doxygen/html/index.html
```

Ouvrez ce fichier dans votre navigateur pour consulter la documentation complète.

## Contenu de la Documentation Générée

### 1. Page Principale
- Description du projet
- Fonctionnalités principales
- Diagramme d'architecture interactif
- Guide de démarrage rapide
- Liens vers les sections détaillées

### 2. Architecture Détaillée
- Vue d'ensemble de l'architecture
- Description des 4 couches (Application, Services, Données, Interface)
- Diagrammes de flux de données
- Patterns de conception
- Gestion des états
- Persistance des données

### 3. Exemples d'Utilisation
- 10 exemples complets avec code
- Création et gestion de chats
- Utilisation du RAG
- Configuration avancée
- Gestion des thèmes
- Intégration QML
- Benchmarking

### 4. Documentation de l'API
- Liste complète des classes
- Hiérarchie des classes
- Documentation détaillée des méthodes
- Description des propriétés et signaux
- Diagrammes de collaboration

### 5. Fichiers Source
- Code source avec surlignage syntaxique
- Commentaires Doxygen intégrés
- Navigation facile entre les fichiers

## Personnalisation

### Ajouter de la Documentation

Pour ajouter de la documentation à un nouveau fichier :

1. **Ajoutez des commentaires Doxygen** :
```cpp
/**
 * @class MaNouvelleClasse
 * @brief Description de la classe
 * 
 * Description détaillée de la classe et de son rôle.
 */
class MaNouvelleClasse {
    /**
     * @brief Description de la méthode
     * @param param Description du paramètre
     * @return Description de la valeur de retour
     */
    int maMethode(int param);
};
```

2. **Ajoutez le fichier à l'INPUT** dans Doxyfile

3. **Générez à nouveau la documentation**

### Modifier la Page Principale

Pour modifier la page principale :

1. **Éditez mainpage.dox** - Pour la page d'accueil
2. **Éditez architecture.dox** - Pour l'architecture détaillée
3. **Éditez examples.md** - Pour les exemples

### Ajouter des Diagrammes

Utilisez la syntaxe @dot pour ajouter des diagrammes :

```cpp
/**
 * @dot
 * digraph exemple {
 *     A -> B;
 *     B -> C;
 *     C -> A;
 * }
 * @enddot
 */
```

## Bonnes Pratiques

### Documentation des Classes
- Utilisez `@class` pour documenter les classes
- Fournissez une description brève et détaillée
- Documentez le rôle et les responsabilités

### Documentation des Méthodes
- Utilisez `@brief` pour une description courte
- Utilisez `@param` pour chaque paramètre
- Utilisez `@return` pour la valeur de retour
- Ajoutez des exemples si nécessaire

### Documentation des Variables
- Utilisez `<--` pour les commentaires inline
- Soyez concis mais descriptif

### Documentation des Signaux
- Documentez quand le signal est émis
- Expliquez ce que le signal représente
- Listez les paramètres du signal

## Résolution des Problèmes

### Problème : Doxygen non trouvé
**Solution** : Installez Doxygen comme décrit ci-dessus

### Problème : Graphviz non trouvé
**Solution** : Installez Graphviz. Les diagrammes ne seront pas générés sans lui.

### Problème : Erreurs de syntaxe dans les fichiers .dox
**Solution** : Vérifiez la syntaxe des commentaires Doxygen

### Problème : Diagrammes non générés
**Solution** : 
1. Vérifiez que Graphviz est installé
2. Assurez-vous que `HAVE_DOT = YES` dans Doxyfile
3. Vérifiez que la syntaxe @dot est correcte

### Problème : Liens brisés
**Solution** : Vérifiez que les références (@ref, @see) pointent vers des éléments existants

## Maintenance

### Mettre à jour la Documentation

Lorsque vous modifiez du code :
1. Mettez à jour les commentaires Doxygen correspondants
2. Si vous ajoutez de nouvelles fonctionnalités, ajoutez des exemples
3. Si vous modifiez l'architecture, mettez à jour architecture.dox
4. Générez à nouveau la documentation

### Vérification de la Documentation

Utilisez la commande suivante pour vérifier les avertissements Doxygen :
```bash
doxygen Doxyfile 2>&1 | grep warning
```

## Ressources

- [Documentation Officielle Doxygen](https://www.doxygen.nl/manual/index.html)
- [Référence des Commandes Doxygen](https://www.doxygen.nl/manual/commands.html)
- [Guide de Style Doxygen](https://www.doxygen.nl/manual/docblocks.html)
- [Graphviz Documentation](https://graphviz.org/doc/info/lang.html)

## Exemple de Sortie

Une fois générée, votre documentation inclura :

- **Page d'accueil** avec description du projet et diagramme d'architecture
- **Section Architecture** avec détails techniques complets
- **Section Exemples** avec du code prêt à l'emploi
- **Documentation API** complète avec diagrammes de classes
- **Index de recherche** pour trouver facilement les éléments
- **Arbre des classes** pour visualiser les hiérarchies

La documentation sera professionnelle, complète et très utile pour les développeurs, utilisateurs et contributeurs.

## Support

Pour toute question sur la documentation :
- Consultez ce guide
- Vérifiez la documentation officielle Doxygen
- Ouvrez une issue sur GitHub si vous trouvez un problème

© 2023 LlamaBot Documentation Team