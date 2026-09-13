# Dependencies

The `dependencies` section declares NXDev and devkitPro portlib packages required by the application.

## Syntax

Both shorthand strings and detailed object representations are supported:

```yaml
dependencies:
  # Shorthand string syntax
  - nxdev.core

  # Expanded object syntax
  - name: switch-sdl2
    version: "^2.28.0"
    optional: false
```

## Duplicate Detection

Declaring the same dependency name multiple times generates diagnostic error `NXM011`.

## Implicit `nxdev.core`

`nxdev.core` is always implicitly available in the NXDev build system. If explicitly declared in `dependencies`, it is tracked and validated without error.
