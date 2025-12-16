# Release workflow and strategy

## Why Tags over a long-lived `dev` branch?

We have decided to use a **Tag-based release system** instead of relying on a long-lived development branch (like `dev` or `release`) for deploying new versions. Here is why:

1.  **Immutability and clarity**:
    -   A **Tag** is a snapshot of the code at a specific point in time. It is immutable and clearly indicates a version (e.g., `v1.0.0`).
    -   Branches move. A `dev` branch is a moving target. Using tags allows us to precisely identify the code corresponding to a release without ambiguity.

2.  **Simplified Git history**:
    -   Using a long-lived branch often leads to complex merge conflicts and diverging histories if not managed perfectly.
    -   With tags, we release from the main branch (or any stable commit) simply by marking it. This keeps the branching model cleaner and more linear.

3.  **CI/CD triggering**:
    -   Our CI pipeline (`.github/workflows/release.yml`) is configured to listen specifically for tags matching the `v*` pattern. This separation prevents accidental deployments that might happen if we were simply pushing to a branch.

4.  **Semantic versioning**:
    -   Tags naturally support Semantic Versioning (SemVer). It is easier to see a list of releases by `git tag` than by looking through commit messages on a branch.

## How the release system works

Our release process is automated using GitHub Actions.

### 1. The trigger
The workflow is defined in `.github/workflows/release.yml`. It triggers **only** when a tag starting with `v` is pushed to the repository.

```yaml
on:
  push:
    tags:
      - "v*"
```

### 2. The process
When you push a tag (e.g., `v1.0.1`):
1.  **Build jobs**: The CI spins up runners for Linux and Windows.
    -   Compiles the Client and Server using `xmake` and `LLVM/Clang`.
    -   Packages the binaries and assets into archives (`.tar.gz` for Linux, `.zip` for Windows).
2.  **Release job**:
    -   Waits for both build jobs to succeed.
    -   Downloads the packaged artifacts.
    -   Creates a **GitHub Release** associated with the tag.
    -   Uploads the binaries to the release page.

## When to create a release (Versioning strategy)

We strictly follow **Semantic Versioning (SemVer)** formatted as `vMAJOR.MINOR.PATCH`.

### 1. Major version (vX.0.0)
Increment the **MAJOR** version when you make **incompatible API changes** or major architectural overhauls.
- **Example**: `v1.0.0` -> `v2.0.0`
- **When**: Rewriting the networking protocol, changing the ECS architecture fundamentally, or removing deprecated features.

### 2. Minor version (v0.X.0)
Increment the **MINOR** version when you add **functionality in a backward compatible manner**.
- **Example**: `v1.1.0` -> `v1.2.0`
- **When**: Adding a new enemy type, implementing a new power-up, or adding a new level.

### 3. Patch version (v0.0.X) - Hotfixes
Increment the **PATCH** version when you make **backward compatible bug fixes**.
- **Example**: `v1.0.1` -> `v1.0.2`
- **When**: Fixing a crash, correcting a texture glitch, or resolving a small logic error.

### Hotfixes
A **Hotfix** is an immediate response to a critical bug in a production release.
- **Workflow**:
    1.  Fix the bug on `main` (or a specific maintenance branch if `main` has moved too far ahead).
    2.  Tag the commit with an incremented **PATCH** version (e.g., specific fix for `v1.2.0` becomes `v1.2.1`).
    3.  Push the tag to trigger the deployment.

## How to create a release

To release a new version of the application, follow these steps:

1.  **Ensure your code is ready** and compiled locally if necessary to verify stability.
2.  **Create a Tag**: Use semantic versioning (e.g., `v1.0.0`, `v1.1.0`, `v2.0.0`).

    ```bash
    git tag v1.0.0
    ```

3.  **Push the Tag**:

    ```bash
    git push origin v1.0.0
    ```

4.  **Monitor the Action**: Go to the "Actions" tab in GitHub to watch the "Release" workflow. Once finished, the new release will appear in the "Releases" section of the repository.
