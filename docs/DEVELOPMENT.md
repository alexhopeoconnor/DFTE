# Development and releases

Released applications should use a public Git tag. While changing DFTE with a sibling library, select a local `symlink://` or `file://` dependency from an ignored PlatformIO override rather than changing tracked application dependencies.

Start a release with `bump-version.sh`. It updates package metadata and canonical installation snippets, then creates the changelog section. Replace its generated TODO with the release summary and update any behavioural documentation before running:

```bash
./scripts/bump-version.sh vMAJOR.MINOR.PATCH
# Replace the generated CHANGELOG TODO with the release summary.
./scripts/check-docs.sh
./scripts/test.sh compile --platform esp8266
./scripts/test.sh compile --platform esp32
./scripts/prepare-release.sh vMAJOR.MINOR.PATCH --tag
```

Push the branch and annotated tag. GitHub Actions repeats the board-free compile checks, validates the package, and creates a GitHub Release from that version’s changelog section. It does not publish to the PlatformIO Registry.

Back to [documentation](README.md) · [project overview](../README.md).
