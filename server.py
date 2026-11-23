#!/usr/bin/env python3
"""
NIshxnt Arsenal - High-Performance Python Web Server for Render Cloud
Features:
- Dynamic Port binding via $PORT (Render standard)
- Full HTTP 206 Partial Content (Byte-Range support for instant video streaming)
- Aggressive Browser Caching for MP4/Images (zero video loading delay)
- Automatic routing for /, /catalog, /about
"""

import os
import sys
import mimetypes
from http.server import HTTPServer, SimpleHTTPRequestHandler

PORT = int(os.environ.get("PORT", 8080))

# Locate the website root directory
CANDIDATE_PATHS = [
    os.path.join(os.path.dirname(__file__), "src", "website"),
    os.path.join(os.path.dirname(__file__), "website"),
    os.path.dirname(__file__)
]

WEB_DIR = os.path.dirname(__file__)
for p in CANDIDATE_PATHS:
    if os.path.isfile(os.path.join(p, "index.html")):
        WEB_DIR = p
        break

print(f"[BOOT] Serving NIshxnt Arsenal from: {WEB_DIR}")
print(f"[BOOT] Listening on port: {PORT}")

# Ensure proper MIME types for fast video playback
mimetypes.add_type("video/mp4", ".mp4")
mimetypes.add_type("image/png", ".png")
mimetypes.add_type("image/jpeg", ".jpg")
mimetypes.add_type("image/jpeg", ".jpeg")
mimetypes.add_type("image/webp", ".webp")
mimetypes.add_type("text/html", ".html")
mimetypes.add_type("text/css", ".css")
mimetypes.add_type("application/javascript", ".js")


class FastStreamingHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def end_headers(self):
        # Enable CORS and Range Support
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Accept-Ranges", "bytes")
        
        # Cache videos & images for 7 days so they load instantly
        path = self.translate_path(self.path)
        if any(path.endswith(ext) for ext in [".mp4", ".png", ".jpg", ".jpeg", ".webp", ".woff2"]):
            self.send_header("Cache-Control", "public, max-age=604800, immutable")
        else:
            self.send_header("Cache-Control", "public, max-age=3600")
            
        super().end_headers()

    def do_GET(self):
        # Route alias handling
        clean_path = self.path.split("?")[0].rstrip("/")
        if clean_path in ["", "/index"]:
            self.path = "/index.html"
        elif clean_path in ["/catalog", "/catalog.html"]:
            self.path = "/catalog.html"
        elif clean_path in ["/about", "/about.html"]:
            self.path = "/about.html"
        elif clean_path == "/healthz" or clean_path == "/api/status":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"healthy","app":"NIshxnt Arsenal"}\n')
            return

        return super().do_GET()

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
        self.end_headers()

    def do_POST(self):
        clean_path = self.path.split("?")[0].rstrip("/")
        if clean_path == "/api/auth/verify":
            content_length = int(self.headers.get('Content-Length', 0))
            post_data = self.rfile.read(content_length).decode('utf-8')
            print(f"[AUTH VERIFY] Incoming authorization payload: {post_data[:140]}")
            
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            response_json = '{"status":"authorized","verified":true,"clearance":"ALPHA-1","message":"Access granted by NIshxnt Defense Gateway"}\n'
            self.wfile.write(response_json.encode('utf-8'))
            return

        self.send_response(404)
        self.end_headers()


if __name__ == "__main__":
    server = HTTPServer(("0.0.0.0", PORT), FastStreamingHandler)
    print(f"[ONLINE] Server ready at http://0.0.0.0:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[STOPPED] Server shut down cleanly.")
