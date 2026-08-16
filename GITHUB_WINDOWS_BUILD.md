# GitHub Windows Build

Paramedic EQ uses GitHub Actions to build the Windows VST3 on a GitHub-hosted Windows runner.

Workflow:
`.github/workflows/windows-vst3.yml`

The workflow builds with Visual Studio 2022 / x64, locates the generated `Paramedic EQ.vst3`, and uploads a ZIP artifact.

## Local repository setup

```bash
git init
git branch -M main
git add .
git commit -m "Paramedic EQ 2.0.5 solo product baseline"
git remote add origin https://github.com/neoe974-tech/ParamedicEQ.git
git push -u origin main
```

After the push, open the repository's **Actions** tab and select **Build Paramedic EQ - Windows VST3**. The completed run provides a downloadable artifact.
