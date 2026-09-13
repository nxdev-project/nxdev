# NACP (Nintendo Application Control Property)

The `nacp` section configures title metadata and system behavior in Horizon OS.

## Fields

| Field | Allowed Values | Default | Description |
| :--- | :--- | :--- | :--- |
| `displayVersion` | `string` | `application.version` | Version displayed in Homebrew Menu / Horizon |
| `startupUserAccount` | `none`, `required`, `optional` | `none` | Prompt user profile selection on startup |
| `userAccountSwitchLock` | `boolean` | `false` | Prevent switching active user profile |
| `screenshot` | `allow`, `deny` | `allow` | Allow or inhibit screen captures |
| `videoCapture` | `disabled`, `manual`, `automatic` | `disabled` | Enable rolling 30s video captures |
| `logoType` | `licensedByNintendo`, `nintendo`, `distributedByNintendo` | `licensedByNintendo` | Nintendo startup splash logo type |
| `parentalControl` | `free`, `restricted` | `free` | Horizon parental control enforcement |
