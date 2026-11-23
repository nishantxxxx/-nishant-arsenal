# NIshxnt Arsenal — Tactical Combat Flow & Classified Catalog

Next-generation tactical automation and classified weaponry showcase engineered with a C++20 core engine, real-time HTTP socket telemetry, and an ultra-modern cybernetic UI.

## Features
- **Zero-Trust Clearance Verification**: Real Google Identity Services (GIS) & Discord OAuth2 1-click authorization.
- **Dynamic 3D Glass Modals**: Ultra-smooth, jitter-free mouse-follow tilt physics with KaTeX/Mermaid styling and HD Pinterest background video loops.
- **Classified Hardware Catalog**: Full tactical weaponry blueprints, interactive weapon cards with video previews, and combat telemetry.
- **Production-Ready Streaming**: Built-in HTTP socket server with byte-range MP4 video streaming, caching, and `/healthz` monitoring.

---

## Deployment on Render (Step-by-Step)

1. **Upload to GitHub**:
   - Push this entire folder to your GitHub repository (e.g., `nishant-arsenal`).

2. **Deploy on Render**:
   - Log in to [dashboard.render.com](https://dashboard.render.com/).
   - Click **New +** $\rightarrow$ **Web Service**.
   - Connect your GitHub repository.
   - Settings:
     - **Name**: `nishant-arsenal`
     - **Runtime**: `Python 3`
     - **Build Command**: `echo 'Ready'`
     - **Start Command**: `python server.py`
     - **Plan**: `Free`
   - Click **Deploy Web Service**.

3. **OAuth Setup**:
   Once you receive your Render live URL (e.g. `https://nishant-arsenal.onrender.com`):
   - In **Google Cloud Console**, add `https://nishant-arsenal.onrender.com` to **Authorised JavaScript origins** and **Authorised redirect URIs**.
   - In **Discord Developer Portal**, add `https://nishant-arsenal.onrender.com` to **OAuth2 Redirects**.

---

## Local Run

```bash
# Run streaming server
python server.py
```
Open your browser at `http://localhost:8080` or `http://127.0.0.1:8080`.
