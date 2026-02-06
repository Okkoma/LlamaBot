# ServerChatStore - Serveur de stockage avec cryptage bout en bout

## Description

Serveur HTTP C++ pour stocker les conversations de LlamaBot dans PostgreSQL avec **cryptage bout en bout**. 
Le serveur ne voit jamais le contenu des conversations en clair - il stocke uniquement les données chiffrées.

## Architecture

- **Cryptage**: AES-256-GCM avec clé dérivée via PBKDF2 (100 000 itérations)
- **Base de données**: PostgreSQL
- **API**: REST HTTP (Qt HttpServer)
- **Sécurité**: Les données sont chiffrées côté client avant envoi, le serveur ne peut jamais les déchiffrer

## Compilation

```bash
cd build
cmake .. -DBUILD_SERVER=ON
make ServerChatStore
```

## Dépendances

- Qt 6.5+ (Core, Network, Sql, **HttpServer**)
- OpenSSL 3.0+ (pour le cryptage)
- PostgreSQL (libpq)
- Driver PostgreSQL pour Qt (`QPSQL`)

## Utilisation

### Démarrer le serveur

```bash
./ServerChatStore \
  --port 8080 \
  --db-host localhost \
  --db-port 5432 \
  --db-name chatstore \
  --db-user postgres \
  --db-password votre_mot_de_passe
```

### Créer la base de données PostgreSQL

```sql
CREATE DATABASE chatstore;
```

Le serveur créera automatiquement les tables au premier démarrage.

## API REST

### GET /conversations
Liste toutes les conversations d'un utilisateur.

**Query params:**
- `user_id`: Identifiant utilisateur

**Headers:**
- `X-User-Id`: Identifiant utilisateur (alternative)

**Réponse:**
```json
{
  "conversations": [
    {
      "id": "uuid",
      "data": "base64...",
      "created_at": "2024-01-01T00:00:00Z",
      "updated_at": "2024-01-01T00:00:00Z"
    }
  ],
  "count": 1
}
```

### GET /conversations/{id}
Récupère une conversation spécifique.

### POST /conversations/sync
Synchronisation batch de plusieurs conversations.

**Body:**
```json
{
  "user_id": "user123",
  "conversations": [
    {
      "id": "uuid",
      "data": "base64..."
    }
  ]
}
```

### DELETE /conversations/{id}
Supprime une conversation (soft delete).

## Schéma PostgreSQL

```sql
CREATE TABLE conversations (
    id UUID PRIMARY KEY,
    user_id VARCHAR(255) NOT NULL,
    encrypted_data TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    deleted_at TIMESTAMP NULL
);

CREATE TABLE sync_metadata (
    conversation_id UUID PRIMARY KEY REFERENCES conversations(id),
    last_sync_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    sync_state VARCHAR(50) DEFAULT 'synced',
    version INTEGER DEFAULT 1
);
```

## Sécurité

- **Cryptage bout en bout**: Le serveur ne peut jamais déchiffrer les conversations
- **Authentification**: Basée sur `user_id` (à améliorer avec JWT/OAuth2)
- **Soft delete**: Les conversations supprimées sont marquées, pas supprimées physiquement

## Notes

- Le module `Qt6::HttpServer` est requis (disponible depuis Qt 6.5)
- Pour Qt < 6.5, utiliser une alternative comme `QTcpServer` + parsing HTTP manuel
