# Configuration of projects

Open `.vscode/settings.json` and put the following content:

```json
{
    ..., // other configurations
    "rust-analyzer.linkedProjects": [
        "ping-pong/rust-version/mutex/Cargo.toml",
        "ping-pong/rust-version/channels/Cargo.toml"
    ]
}
```
