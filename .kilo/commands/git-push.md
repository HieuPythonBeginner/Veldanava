# Git Push Guide

## Standard build & push:
```bash
# Build
cmake --build build --target veldanc

# Add all changes
git add .

# Commit
git commit -m "your message"

# Push
git push origin main
```

## Initial setup (once):
```bash
ssh-keygen -t ed25519 -C "email@example.com"
# Add public key to GitHub Settings → SSH and GPG keys

git remote set-url origin git@github.com:HieuPythonBeginner/Veldanava.git
```