# Releasing Abrade

Abrade releases are GitHub-native. A semver tag such as `v0.3.0` publishes a
GitHub Release with Linux, macOS, and Windows archives plus `SHA256SUMS`.

## Version Contract

- `CMakeLists.txt` is the source of the project version.
- Release tags must match `vMAJOR.MINOR.PATCH`.
- The tag version must equal the CMake project version without the leading `v`.
- Example: `project(abrade VERSION 0.3.0 LANGUAGES CXX)` releases from tag
  `v0.3.0`.

## Rehearse A Release

Use the `Release` workflow manually before pushing a real release tag:

1. Open GitHub Actions.
2. Run the `Release` workflow on the target branch.
3. Set `tag` to the intended release tag, such as `v0.3.0`.
4. Leave `publish_release` unchecked.
5. Confirm the workflow produces `release-assets-TAG`.
6. Download the artifact and verify `SHA256SUMS`.

The rehearsal uses hosted Linux, macOS, and Windows runners. Ordinary PRs remain
on the bounded Marvin/fork-safe CI path.

Before rehearsing, confirm the user-facing docs match the release:

- README install and quick-start commands are current.
- `docs/index.md`, `docs/usage.md`, and `docs/patterns.md` match CLI behavior.
- `docs/building.md` and this release guide match the workflow files.
- Any historical notes stay in `docs/history.md`, not in the current install
  path.

## Publish A Release

After the rehearsal passes and `master` is green:

```
git switch master
git pull --ff-only origin master
git tag -a v0.3.0 -m "Abrade v0.3.0"
git push origin v0.3.0
```

The tag push starts the `Release` workflow. The workflow:

- validates the tag against the CMake version;
- configures, builds, tests, and smoke-tests each hosted platform;
- stages the install tree with `cmake --install`;
- creates platform archives;
- generates `SHA256SUMS`;
- publishes the GitHub Release with all release assets.

## Verify Assets

Linux:

```
sha256sum -c SHA256SUMS --ignore-missing
```

macOS:

```
shasum -a 256 -c SHA256SUMS
```

Windows PowerShell:

```
Get-FileHash .\abrade-0.3.0-windows-x64.zip -Algorithm SHA256
```

Compare the digest to the matching line in `SHA256SUMS`.

## Bad Release Rollback

If a release tag publishes bad assets:

1. Delete the GitHub Release from the Releases page or with `gh release delete`.
2. Delete the remote tag only after confirming no users should consume it:

   ```
   git push origin :refs/tags/v0.3.0
   ```

3. Delete the local tag:

   ```
   git tag -d v0.3.0
   ```

4. Fix the issue on a PR.
5. Rehearse again.
6. Publish a new patch tag.

Prefer a new patch version over rewriting a published tag after users may have
downloaded artifacts.
