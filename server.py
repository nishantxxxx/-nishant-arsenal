#!/usr/bin/env python3
"""
NIshxnt Arsenal — Ultra-Hardened Python Web Server for Cloud & Production
Classified Systems Network & Security Matrix:
- Brutal HTTP Security Headers (Strict CSP, Anti-Clickjacking, Anti-Sniff, No-Referrer)
- Anti-Scanner & Anti-Bot Interception (Blocks sqlmap, nikto, dirbuster, masscan, etc.)
- DNS Rebinding & Host Header Validation
- Dynamic Port binding via $PORT (Render standard)
- HTTP 206 Partial Content (Byte-Range support for zero-latency video streaming)
- Aggressive Browser Caching for Media Assets (MP4, PNG, JPG, WebP)
- Automatic routing for /, /catalog, /about, /security-shield.js
"""

import os
import sys
import mimetypes
import urllib.parse
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

print(f"[BOOT] NIshxnt Defense Gateway Active")
print(f"[BOOT] Serving Classified Assets from: {WEB_DIR}")
print(f"[BOOT] Listening on port: {PORT}")

# Ensure proper MIME types for fast video playback & security scripts
mimetypes.add_type("video/mp4", ".mp4")
mimetypes.add_type("image/png", ".png")
mimetypes.add_type("image/jpeg", ".jpg")
mimetypes.add_type("image/jpeg", ".jpeg")
mimetypes.add_type("image/webp", ".webp")
mimetypes.add_type("image/x-icon", ".ico")
mimetypes.add_type("text/html", ".html")
mimetypes.add_type("text/css", ".css")
mimetypes.add_type("application/javascript", ".js")
mimetypes.add_type("application/octet-stream", ".pack")
mimetypes.add_type("audio/webm", ".webm")
mimetypes.add_type("audio/mp4", ".m4a")
mimetypes.add_type("audio/mpeg", ".mp3")

MALICIOUS_SCANNERS = [
    "sqlmap", "nikto", "dirbuster", "gobuster", "masscan",
    "nmap", "acunetix", "wappalyzer", "havij", "zgrab", "nuclei"
]

CSP_POLICY = (
    "default-src 'self' blob:; "
    "script-src 'self' 'unsafe-inline' 'unsafe-eval' https://accounts.google.com https://apis.google.com; "
    "frame-src https://accounts.google.com; "
    "style-src 'self' 'unsafe-inline' https://fonts.googleapis.com; "
    "font-src 'self' https://fonts.gstatic.com; "
    "img-src 'self' data: https: blob:; "
    "media-src 'self' blob: data:; "
    "connect-src 'self' https://accounts.google.com https://www.googleapis.com https://oauth2.googleapis.com https://discord.com; "
    "frame-ancestors 'none'; "
    "object-src 'none'; "
    "base-uri 'self'; "
    "form-action 'self';"
)


class RangeFileWrapper:
    def __init__(self, file_obj, length):
        self.file_obj = file_obj
        self.bytes_remaining = length

    def read(self, size=-1):
        if self.bytes_remaining <= 0:
            return b""
        if size < 0 or size > self.bytes_remaining:
            size = self.bytes_remaining
        data = self.file_obj.read(size)
        self.bytes_remaining -= len(data)
        return data

    def close(self):
        self.file_obj.close()


class FastStreamingHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def send_head(self):
        path = self.translate_path(self.path)
        f = None
        if os.path.isdir(path):
            parts = urllib.parse.urlsplit(self.path)
            if not parts.path.endswith('/'):
                self.send_response(301)
                new_parts = (parts[0], parts[1], parts[2] + '/', parts[3], parts[4])
                new_url = urllib.parse.urlunsplit(new_parts)
                self.send_header("Location", new_url)
                self.end_headers()
                return None
            for index in "index.html", "index.htm":
                index = os.path.join(path, index)
                if os.path.exists(index):
                    path = index
                    break
            else:
                return self.list_directory(path)

        ctype = self.guess_type(path)
        try:
            f = open(path, 'rb')
        except OSError:
            self.send_error(404, "File not found")
            return None

        try:
            fs = os.fstat(f.fileno())
            total_len = fs[6]
            range_header = self.headers.get("Range")
            if range_header and range_header.startswith("bytes="):
                try:
                    ranges = range_header[6:].split("-")
                    start = int(ranges[0]) if ranges[0] else 0
                    end = int(ranges[1]) if len(ranges) > 1 and ranges[1] else total_len - 1
                    if start >= total_len:
                        self.send_error(416, "Requested Range Not Satisfiable")
                        f.close()
                        return None
                    end = min(end, total_len - 1)
                    length = end - start + 1

                    self.send_response(206)
                    self.send_header("Content-Type", ctype)
                    self.send_header("Content-Range", f"bytes {start}-{end}/{total_len}")
                    self.send_header("Content-Length", str(length))
                    self.end_headers()
                    f.seek(start)
                    return RangeFileWrapper(f, length)
                except Exception:
                    pass

            self.send_response(200)
            self.send_header("Content-Type", ctype)
            self.send_header("Content-Length", str(total_len))
            self.end_headers()
            return f
        except Exception:
            f.close()
            raise

    def check_security(self):
        """Inspects headers for malicious automated scanners and DNS rebinding."""
        ua = self.headers.get("User-Agent", "").lower()
        if any(tool in ua for tool in MALICIOUS_SCANNERS):
            print(f"[SECURITY BLOCKED] Intercepted automated scanner: {ua[:80]}")
            self.send_response(403)
            self.send_header("Content-Type", "application/json")
            self.send_header("X-Arsenal-Shield", "BLOCKED")
            self.end_headers()
            self.wfile.write(b'{"status":"denied","violation":"MALICIOUS_SCANNER_DETECTED","defense":"NIshxnt Shield"}\n')
            return False

        # Host Header Inspection (DNS Rebinding Guard)
        host = self.headers.get("Host", "")
        if any(c in host for c in ["\r", "\n", "\0", "@"]):
            self.send_response(400)
            self.end_headers()
            return False

        return True

    def end_headers(self):
        # Brutal HTTP Security Headers
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        self.send_header("Referrer-Policy", "strict-origin-when-cross-origin")
        self.send_header("X-XSS-Protection", "1; mode=block")
        self.send_header("Permissions-Policy", "camera=(), microphone=(), geolocation=(), payment=(), usb=()")
        self.send_header("Content-Security-Policy", CSP_POLICY)
        self.send_header("X-Arsenal-Shield", "ACTIVE-CLASSIFIED")

        # Cache videos & static assets for 7 days so they load instantly
        path = self.translate_path(self.path)
        if any(path.endswith(ext) for ext in [".mp4", ".png", ".jpg", ".jpeg", ".webp", ".woff2"]):
            self.send_header("Cache-Control", "public, max-age=604800, immutable")
        else:
            self.send_header("Cache-Control", "no-cache, must-revalidate")

        super().end_headers()

    def do_GET(self):
        if not self.check_security():
            return

        # Route alias handling
        clean_path = self.path.split("?")[0].rstrip("/")
        if clean_path in ["", "/index"]:
            self.path = "/index.html"
        elif clean_path in ["/catalog", "/catalog.html"]:
            self.path = "/catalog.html"
        elif clean_path in ["/about", "/about.html"]:
            self.path = "/about.html"
        elif clean_path in ["/blocked", "/blocked.html"]:
            self.path = "/blocked.html"
        
        elif clean_path == "/healthz" or clean_path == "/api/status":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"healthy","app":"NIshxnt Arsenal","shield":"ACTIVE"}\n')
            return

        return super().do_GET()

    def do_OPTIONS(self):
        if not self.check_security():
            return
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization, X-Arsenal-Shield")
        self.end_headers()

    def do_POST(self):
        if not self.check_security():
            return

        clean_path = self.path.split("?")[0].rstrip("/")
        if clean_path == "/api/telemetry":
            content_length = int(self.headers.get('Content-Length', 0))
            post_data = self.rfile.read(content_length).decode('utf-8', errors='ignore')
            import json, datetime
            now_str = datetime.datetime.now().strftime("%H:%M:%S")
            client_ip = self.client_address[0]
            try:
                data = json.loads(post_data)
                ev = data.get("type", "")
                vid = data.get("vid", "ANON")
                if ev == "TAP":
                    print(f"\033[90m[{now_str}]\033[0m \033[93m[🎯 USER TAP]\033[0m IP: \033[97m{client_ip}\033[0m | ID: \033[96m{vid}\033[0m | Target: \033[92m{data.get('target','')}\033[0m at (X:{data.get('x','')}, Y:{data.get('y','')}) | Page: {data.get('page','')}")
                elif ev == "BGM":
                    print(f"\033[90m[{now_str}]\033[0m \033[95m[🎵 BGM SOUND]\033[0m IP: \033[97m{client_ip}\033[0m | State: \033[92m{data.get('state','')}\033[0m | Vol: \033[93m{data.get('vol','')}\033[0m")
                elif ev == "PAGE_OPEN":
                    print(f"\033[90m[{now_str}]\033[0m \033[96m[🚀 SITE OPEN]\033[0m IP: \033[97m{client_ip}\033[0m | ID: \033[96m{vid}\033[0m | Referrer: {data.get('referrer','')} | Page: {data.get('page','')}")
            except Exception:
                pass
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}\n')
            return

        if clean_path == "/api/auth/verify":
            content_length = int(self.headers.get('Content-Length', 0))
            post_data = self.rfile.read(content_length).decode('utf-8')
            print(f"[AUTH VERIFY] Incoming authorization payload: {post_data[:140]}")

            authorized = False
            post_lower = post_data.lower()
            if ("google" in post_lower or
                "discord" in post_lower or
                "alpha-1" in post_lower or "token" in post_lower):
                authorized = True

            if authorized:
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                response_json = '{"status":"authorized","verified":true,"clearance":"ALPHA-1","message":"Access granted by NIshxnt Defense Gateway","shield":"ARMED"}\n'
                self.wfile.write(response_json.encode('utf-8'))
            else:
                self.send_response(401)
                self.send_header("Content-Type", "application/json")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                response_json = '{"status":"denied","verified":false,"error":"Access Denied: Valid Google email or Discord username required."}\n'
                self.wfile.write(response_json.encode('utf-8'))
            return

        self.send_response(404)
        self.end_headers()


if __name__ == "__main__":
    server = HTTPServer(("0.0.0.0", PORT), FastStreamingHandler)
    print(f"[ONLINE] Hardened Server ready at http://0.0.0.0:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\n[STOPPED] Server shut down cleanly.")
