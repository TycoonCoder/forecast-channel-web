# Deployment status

**Live** at https://tycooncoder.github.io/forecast-channel-web/
Repo: https://github.com/TycoonCoder/forecast-channel-web

`deploy.sh` is idempotent — re-run it any time to ship updates:

```bash
bash deploy.sh
```

It commits, pushes to `main`, and verifies Pages is enabled on `main` / root.
The first Pages build after enabling can take up to a minute.
