# Application Metadata & Localization

The `application` section defines the core identity of the homebrew project.

## Fields

| Field | Type | Required | Default | Description |
| :--- | :--- | :--- | :--- | :--- |
| `name` | `string` | **Yes** | — | Display name of the application (1..255 chars) |
| `author` | `string` | **Yes** | — | Author name, developer or studio (1..255 chars) |
| `version` | `string` | No | `"1.0.0"` | Human-readable version string |
| `titleId` | `string` | No | `None` | 16-digit hexadecimal Nintendo Title ID |
| `description` | `string` | No | `""` | Optional short summary |
| `localized` | `map` | No | `{}` | Map of language codes to localized titles |

## Title ID Formatting & Validation

Title IDs must be exactly 16 hexadecimal digits (with optional `0x` prefix).
Examples of valid Title IDs:
- `"0100000000001000"`
- `"0100ABCD12340000"`
- `"0x0100000000001000"`

Invalid Title IDs will produce diagnostic error `NXM007`.

## Localization

Applications can specify language-specific overrides for name, author, and description:

```yaml
application:
  name: "Chess NX"
  author: "BoardGames Studio"
  version: "1.0.0"
  localized:
    ja:
      name: "チェス NX"
      author: "ボードゲームスタジオ"
    pt-BR:
      name: "Xadrez NX"
      author: "Estúdio Jogos de Tabuleiro"
```

Supported language codes include: `en-US`, `en-GB`, `ja`, `fr`, `fr-CA`, `de`, `es`, `es-419`, `it`, `nl`, `pt`, `pt-BR`, `ru`, `zh-Hans`, `zh-Hant`, `ko`.
