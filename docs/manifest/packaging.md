# Packaging Configuration

The `packaging` section specifies target format options for `nxdev pack`.

## Fields

```yaml
packaging:
  defaultFormat: nro         # 'nro' | 'nsp'
  nro:
    enabled: true            # Generate NRO bundle
  nsp:
    enabled: false           # Generate installable NSP package
```

## Local vs. Project Configuration

Configuration stored in `nxapp.yaml` should be shareable across your team or repository. Machine-specific paths (such as local `nxlink` IP addresses or developer keys) should not be committed to source control.
