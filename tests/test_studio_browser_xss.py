#!/usr/bin/env python3
"""
Browser-based XSS mitigation validation for ELO Content Studio (web/admin/app.js).
Launches an isolated HTTP server with malicious XSS editorial payloads,
opens headless Firefox with an ephemeral profile, and confirms that
no injected scripts or onerror handlers are executed in the browser DOM.
"""

import http.server
import json
import os
import shutil
import socketserver
import subprocess
import sys
import tempfile
import threading
import time

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
WEB_DIR = os.path.join(REPO_ROOT, "web", "admin")

TEST_PAYLOAD = {
    "atoms": [
        {
            "content_id": "atom_xss_probe",
            "type": "entity",
            "title": "<script>window.__xss_title_executed = true;</script>",
            "canonical_name": "XSS Probe <img src=\"nonexistent_probe_img.png\" onerror=\"window.__xss_img_executed = true;\">",
            "subtitle": "Subtitle <svg onload=\"window.__xss_svg_executed = true;\">",
            "canonical_facts": [
                {
                    "fact_id": "fact_01",
                    "statement": "</textarea><script>window.__xss_fact_executed = true;</script>",
                    "confidence": "high",
                    "source_ids": ["src_01"]
                }
            ],
            "metadata": {
                "danger_key<script>": "<b onmouseover=\"window.__xss_b_executed = true;\">value</b>"
            }
        }
    ],
    "relations": [
        {
            "relation_id": "rel_probe",
            "type": "<script>window.__xss_rel_type = true;</script>",
            "from": "<script>window.__xss_rel_from = true;</script>",
            "to": "<script>window.__xss_rel_to = true;</script>",
            "description": "<img src=x onerror=\"window.__xss_rel_img = true;\">"
        }
    ],
    "recipes": [
        {
            "recipe_id": "rec_probe",
            "name": "<script>window.__xss_rec_name = true;</script>",
            "description": "<script>window.__xss_rec_desc = true;</script>",
            "step_count": 1
        }
    ]
}

report_received = None
report_event = threading.Event()


class StudioSecurityTestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEB_DIR, **kwargs)

    def do_POST(self):
        global report_received
        print(f"[HTTP POST] {self.path}")
        if self.path == "/api/test-report":
            length = int(self.headers.get("Content-Length", 0))
            data = self.rfile.read(length)
            report_received = json.loads(data.decode("utf-8"))
            report_event.set()
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(b'{"status":"ok"}')
        else:
            self.send_response(404)
            self.end_headers()

    def do_GET(self):
        if self.path == "/api/status":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Content-Security-Policy", "default-src 'self' 'unsafe-inline'; script-src 'self' 'unsafe-inline';")
            self.end_headers()
            self.wfile.write(json.dumps({"kiosk_online": False}).encode("utf-8"))
        elif self.path == "/api/manifest":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"bundle_id": "test_bundle", "version": "1.0.0"}).encode("utf-8"))
        elif self.path == "/api/catalog":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps(TEST_PAYLOAD).encode("utf-8"))
        elif self.path == "/api/packages":
            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.end_headers()
            self.wfile.write(json.dumps({"packages": []}).encode("utf-8"))
        elif self.path == "/test-hook.js":
            self.send_test_hook_js()
        elif self.path == "/" or self.path == "/index.html":
            self.send_index_with_test_hook()
        else:
            super().do_GET()

    def send_test_hook_js(self):
        hook_js = """
// Pre-seed authentication token
sessionStorage.setItem('eloAdminToken', '0123456789abcdef0123456789abcdef');
window.prompt = function() { return '0123456789abcdef0123456789abcdef'; };

window.onerror = function(msg, url, line) {
  fetch('/api/test-report', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ error: msg + ' at ' + url + ':' + line, passed: false })
  });
};

setTimeout(async () => {
  try {
    const report = {
      title_xss: !!window.__xss_title_executed,
      img_xss: !!window.__xss_img_executed,
      svg_xss: !!window.__xss_svg_executed,
      fact_xss: !!window.__xss_fact_executed,
      b_xss: !!window.__xss_b_executed,
      rel_type_xss: !!window.__xss_rel_type,
      rel_from_xss: !!window.__xss_rel_from,
      rel_to_xss: !!window.__xss_rel_to,
      rel_img_xss: !!window.__xss_rel_img,
      rec_name_xss: !!window.__xss_rec_name,
      rec_desc_xss: !!window.__xss_rec_desc,
      atom_grid_text: document.getElementById('atoms-grid')?.textContent || '',
      card_count: document.querySelectorAll('.atom-card').length
    };

    report.passed = (
      !report.title_xss && !report.img_xss && !report.svg_xss &&
      !report.fact_xss && !report.b_xss && !report.rel_type_xss &&
      !report.rel_from_xss && !report.rel_to_xss && !report.rel_img_xss &&
      !report.rec_name_xss && !report.rec_desc_xss &&
      report.card_count >= 1 &&
      report.atom_grid_text.includes("<script>window.__xss_title_executed = true;</script>")
    );

    await fetch('/api/test-report', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(report)
    });
  } catch (err) {
    await fetch('/api/test-report', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ error: err.toString(), passed: false })
    });
  }
}, 1500);
"""
        self.send_response(200)
        self.send_header("Content-Type", "application/javascript; charset=utf-8")
        self.end_headers()
        self.wfile.write(hook_js.encode("utf-8"))

    def send_index_with_test_hook(self):
        index_path = os.path.join(WEB_DIR, "index.html")
        with open(index_path, "r", encoding="utf-8") as f:
            html = f.read()

        head_seed = """<head>
  <script src="/test-hook.js"></script>"""
        html = html.replace("<head>", head_seed, 1)

        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(html.encode("utf-8"))

    def log_message(self, format, *args):
        print(f"[HTTP] {format % args}")


def main():
    server = socketserver.TCPServer(("127.0.0.1", 0), StudioSecurityTestHandler)
    port = server.server_address[1]
    server_thread = threading.Thread(target=server.serve_forever, daemon=True)
    server_thread.start()
    print(f"[TEST] Test HTTP server listening on http://127.0.0.1:{port}")

    firefox_bin = shutil.which("firefox")
    if not firefox_bin:
        print("[SKIP] Firefox not found on system; browser DOM test skipped.")
        server.shutdown()
        return 0

    temp_profile = tempfile.mkdtemp(prefix="elo_ff_test_")
    url = f"http://127.0.0.1:{port}/"

    cmd = [
        firefox_bin,
        "--headless",
        "--no-remote",
        "-profile", temp_profile,
        url
    ]

    print(f"[TEST] Spawning headless Firefox to test {url}...")
    proc = subprocess.Popen(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    try:
        # Wait up to 15 seconds for test report from browser
        if not report_event.wait(timeout=15.0):
            print("[FAIL] Timeout waiting for browser report from test harness.")
            return 1

        print("[TEST] Report received from browser:")
        print(json.dumps(report_received, indent=2))

        assert report_received is not None, "Report must not be None"
        assert report_received["passed"] is True, f"Browser test assertions failed: {report_received}"
        assert not report_received["title_xss"], "Title script executed in browser!"
        assert not report_received["img_xss"], "Image onerror executed in browser!"
        assert not report_received["svg_xss"], "SVG onload executed in browser!"
        assert not report_received["fact_xss"], "Fact textarea script executed in browser!"
        assert report_received["card_count"] == 1, "Atom card must be rendered in DOM"

        print("\n[SUCCESS] Browser proof verified: editorial HTML/JS rendered harmlessly as text; zero script executions.\n")
        return 0

    finally:
        try:
            proc.terminate()
            proc.wait(timeout=3.0)
        except Exception:
            proc.kill()
        server.shutdown()
        shutil.rmtree(temp_profile, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
