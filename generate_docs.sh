#!/bin/bash

# Script pour générer la documentation Doxygen

echo "Génération de la documentation Doxygen pour LlamaBot..."

# Vérifier si Doxygen est installé
if ! command -v doxygen &> /dev/null; then
    echo "Erreur : Doxygen n'est pas installé."
    echo "Veuillez installer Doxygen avant de générer la documentation."
    echo "Sous Ubuntu/Debian : sudo apt-get install doxygen graphviz"
    echo "Sous Fedora : sudo dnf install doxygen graphviz"
    echo "Sous macOS : brew install doxygen graphviz"
    exit 1
fi

# Vérifier si Graphviz est installé (nécessaire pour les diagrammes)
if ! command -v dot &> /dev/null; then
    echo "Avertissement : Graphviz (dot) n'est pas installé."
    echo "Les diagrammes ne seront pas générés."
    echo "Pour installer Graphviz :"
    echo "Sous Ubuntu/Debian : sudo apt-get install graphviz"
    echo "Sous Fedora : sudo dnf install graphviz"
    echo "Sous macOS : brew install graphviz"
fi

# Créer le répertoire de sortie s'il n'existe pas
mkdir -p Docs/doxygen/html

# Générer la documentation
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo "Documentation générée avec succès dans docs/doxygen/html"
    echo "Ouvrez index.html dans votre navigateur pour consulter la documentation."
else
    echo "Erreur lors de la génération de la documentation."
    exit 1
fi