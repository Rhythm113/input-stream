#include "httplib.h"
#include "input_injector.h"

#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

// ==================== Embedded Web UI ====================
static const char* HTML_PAGE = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no, maximum-scale=1.0">
<title>Input Stream - Remote Control</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&display=swap');

  * { margin: 0; padding: 0; box-sizing: border-box; -webkit-tap-highlight-color: transparent; }

  :root {
    --bg-primary: #0a0a0f;
    --bg-secondary: #12121a;
    --bg-card: rgba(255,255,255,0.04);
    --bg-card-hover: rgba(255,255,255,0.07);
    --border: rgba(255,255,255,0.08);
    --text-primary: #e8e8ed;
    --text-secondary: #8888a0;
    --accent: #6c5ce7;
    --accent-glow: rgba(108,92,231,0.3);
    --accent2: #a29bfe;
    --success: #00cec9;
    --danger: #ff6b6b;
    --radius: 16px;
    --radius-sm: 10px;
  }

  html, body {
    height: 100%; width: 100%;
    overflow: hidden;
    background: var(--bg-primary);
    color: var(--text-primary);
    font-family: 'Inter', -apple-system, sans-serif;
    touch-action: none;
    user-select: none;
    -webkit-user-select: none;
  }

  .app {
    display: flex; flex-direction: column;
    height: 100%; width: 100%;
    max-width: 600px; margin: 0 auto;
    padding: 8px;
    gap: 8px;
  }

  /* Header */
  .header {
    display: flex; align-items: center; justify-content: space-between;
    padding: 10px 16px;
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    backdrop-filter: blur(20px);
    flex-shrink: 0;
  }
  .header .logo {
    display: flex; align-items: center; gap: 10px;
  }
  .header .logo-icon {
    width: 32px; height: 32px;
    background: linear-gradient(135deg, var(--accent), var(--accent2));
    border-radius: 8px;
    display: flex; align-items: center; justify-content: center;
    font-size: 16px;
  }
  .header h1 {
    font-size: 15px; font-weight: 600;
    background: linear-gradient(135deg, var(--text-primary), var(--accent2));
    -webkit-background-clip: text; -webkit-text-fill-color: transparent;
  }
  .header .status {
    display: flex; align-items: center; gap: 6px;
    font-size: 11px; color: var(--success); font-weight: 500;
  }
  .header .status::before {
    content: ''; width: 6px; height: 6px;
    background: var(--success); border-radius: 50%;
    box-shadow: 0 0 8px var(--success);
    animation: pulse 2s infinite;
  }
  @keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.4; }
  }

  /* Trackpad */
  .trackpad-container {
    flex: 1; min-height: 0;
    display: flex; flex-direction: column; gap: 6px;
  }
  .trackpad {
    flex: 1;
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    position: relative;
    overflow: hidden;
    cursor: crosshair;
    transition: border-color 0.2s, box-shadow 0.2s;
  }
  .trackpad.active {
    border-color: var(--accent);
    box-shadow: 0 0 30px var(--accent-glow), inset 0 0 30px rgba(108,92,231,0.05);
  }
  .trackpad-grid {
    position: absolute; inset: 0;
    background-image:
      linear-gradient(rgba(255,255,255,0.02) 1px, transparent 1px),
      linear-gradient(90deg, rgba(255,255,255,0.02) 1px, transparent 1px);
    background-size: 40px 40px;
    pointer-events: none;
  }
  .trackpad-label {
    position: absolute; top: 50%; left: 50%;
    transform: translate(-50%, -50%);
    color: var(--text-secondary);
    font-size: 13px; font-weight: 400;
    opacity: 0.4;
    pointer-events: none;
    text-align: center;
    line-height: 1.6;
  }
  .trackpad-ripple {
    position: absolute; width: 40px; height: 40px;
    border-radius: 50%;
    background: radial-gradient(circle, var(--accent-glow), transparent);
    transform: translate(-50%, -50%) scale(0);
    pointer-events: none;
    animation: ripple 0.6s ease-out forwards;
  }
  @keyframes ripple {
    to { transform: translate(-50%, -50%) scale(3); opacity: 0; }
  }

  /* Mouse Buttons */
  .mouse-buttons {
    display: flex; gap: 6px;
    flex-shrink: 0;
  }
  .mouse-btn {
    flex: 1;
    padding: 14px;
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: var(--radius-sm);
    color: var(--text-primary);
    font-family: inherit;
    font-size: 12px; font-weight: 500;
    text-align: center;
    cursor: pointer;
    transition: all 0.15s;
    display: flex; align-items: center; justify-content: center; gap: 6px;
  }
  .mouse-btn:active {
    background: var(--accent);
    border-color: var(--accent);
    transform: scale(0.97);
  }
  .mouse-btn.middle {
    flex: 0.6;
  }

  /* Scroll area */
  .scroll-strip {
    width: 100%; height: 42px;
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: var(--radius-sm);
    display: flex; align-items: center; justify-content: center;
    color: var(--text-secondary);
    font-size: 11px; font-weight: 400;
    flex-shrink: 0;
    position: relative;
    overflow: hidden;
  }
  .scroll-strip.active {
    border-color: var(--accent);
  }

  /* Keyboard */
  .keyboard-section {
    flex-shrink: 0;
    display: flex; flex-direction: column; gap: 6px;
  }
  .keyboard-input-wrapper {
    position: relative;
  }
  .keyboard-input {
    width: 100%;
    padding: 14px 16px;
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: var(--radius-sm);
    color: var(--text-primary);
    font-family: inherit;
    font-size: 14px;
    outline: none;
    transition: border-color 0.2s;
    caret-color: var(--accent);
  }
  .keyboard-input::placeholder {
    color: var(--text-secondary);
    opacity: 0.5;
  }
  .keyboard-input:focus {
    border-color: var(--accent);
    box-shadow: 0 0 20px var(--accent-glow);
  }

  /* Special keys */
  .special-keys {
    display: flex; gap: 4px;
    flex-wrap: wrap;
  }
  .skey {
    padding: 10px 6px;
    min-width: 40px;
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: 8px;
    color: var(--text-secondary);
    font-family: inherit;
    font-size: 10px; font-weight: 500;
    text-align: center;
    cursor: pointer;
    transition: all 0.15s;
    flex: 1;
    white-space: nowrap;
  }
  .skey:active {
    background: var(--accent);
    color: white;
    border-color: var(--accent);
    transform: scale(0.95);
  }
  .skey.wide { flex: 1.6; }
  .skey.accent-key {
    background: rgba(108,92,231,0.15);
    border-color: rgba(108,92,231,0.3);
    color: var(--accent2);
  }

  /* Feedback toast */
  .toast {
    position: fixed; bottom: 80px; left: 50%;
    transform: translateX(-50%) translateY(20px);
    background: var(--bg-secondary);
    border: 1px solid var(--border);
    border-radius: 8px;
    padding: 8px 16px;
    font-size: 12px;
    color: var(--text-secondary);
    opacity: 0;
    transition: all 0.3s;
    pointer-events: none;
    z-index: 100;
  }
  .toast.show {
    opacity: 1;
    transform: translateX(-50%) translateY(0);
  }

  /* Responsive */
  @media (max-height: 600px) {
    .app { gap: 4px; padding: 4px; }
    .header { padding: 6px 12px; }
    .header .logo-icon { width: 24px; height: 24px; font-size: 12px; }
    .header h1 { font-size: 13px; }
    .mouse-btn { padding: 10px; }
    .skey { padding: 8px 4px; font-size: 9px; }
    .keyboard-input { padding: 10px 12px; font-size: 13px; }
    .scroll-strip { height: 34px; }
  }

  @media (min-width: 768px) {
    .app { max-width: 480px; padding: 16px; gap: 10px; }
  }
</style>
</head>
<body>

<div class="app">
  <!-- Header -->
  <div class="header">
    <div class="logo">
      <div class="logo-icon">🎮</div>
      <h1>Input Stream</h1>
    </div>
    <div class="status">Connected</div>
  </div>

  <!-- Trackpad -->
  <div class="trackpad-container">
    <div class="trackpad" id="trackpad">
      <div class="trackpad-grid"></div>
      <div class="trackpad-label">Drag to move cursor<br>Tap to click</div>
    </div>
  </div>

  <!-- Mouse Buttons -->
  <div class="mouse-buttons">
    <button class="mouse-btn" id="btnLeft">🖱 Left</button>
    <button class="mouse-btn middle" id="btnMiddle">⬤</button>
    <button class="mouse-btn" id="btnRight">Right 🖱</button>
  </div>

  <!-- Scroll -->
  <div class="scroll-strip" id="scrollStrip">↕ Swipe to Scroll</div>

  <!-- Keyboard Input -->
  <div class="keyboard-section">
    <div class="keyboard-input-wrapper">
      <input type="text" class="keyboard-input" id="kbInput" placeholder="Type here to send keystrokes..." autocomplete="off" autocorrect="off" autocapitalize="off" spellcheck="false">
    </div>
    <div class="special-keys">
      <button class="skey accent-key" data-key="enter">Enter ↵</button>
      <button class="skey" data-key="tab">Tab ⇥</button>
      <button class="skey" data-key="backspace">← Bksp</button>
      <button class="skey" data-key="escape">Esc</button>
      <button class="skey" data-key="delete">Del</button>
    </div>
    <div class="special-keys">
      <button class="skey" data-key="up">▲</button>
      <button class="skey" data-key="down">▼</button>
      <button class="skey" data-key="left">◀</button>
      <button class="skey" data-key="right">▶</button>
      <button class="skey wide" data-key="space">Space</button>
    </div>
    <div class="special-keys">
      <button class="skey" data-key="ctrl+a">^A</button>
      <button class="skey" data-key="ctrl+c">^C</button>
      <button class="skey" data-key="ctrl+v">^V</button>
      <button class="skey" data-key="ctrl+z">^Z</button>
      <button class="skey" data-key="ctrl+x">^X</button>
      <button class="skey" data-key="alt+tab">Alt⇥</button>
      <button class="skey" data-key="ctrl+shift+escape">Task</button>
    </div>
  </div>
</div>

<div class="toast" id="toast"></div>

<script>
(function() {
  const THROTTLE_MS = 16; // ~60fps
  const API_BASE = '';

  // ---- Helpers ----
  function post(path, body) {
    fetch(API_BASE + path, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(body)
    }).catch(() => {});
  }

  let toastTimer;
  function showToast(msg) {
    const t = document.getElementById('toast');
    t.textContent = msg;
    t.classList.add('show');
    clearTimeout(toastTimer);
    toastTimer = setTimeout(() => t.classList.remove('show'), 800);
  }

  // Grab keyboard input early so all handlers can blur it
  const kbInput = document.getElementById('kbInput');

  // ---- TRACKPAD ----
  const trackpad = document.getElementById('trackpad');
  let tpActive = false;
  let lastX = 0, lastY = 0;
  let lastSend = 0;
  let tapStart = 0;
  let tapX = 0, tapY = 0;
  let moved = false;
  let fingerCount = 0;

  function addRipple(x, y) {
    const r = document.createElement('div');
    r.className = 'trackpad-ripple';
    r.style.left = x + 'px';
    r.style.top = y + 'px';
    trackpad.appendChild(r);
    setTimeout(() => r.remove(), 600);
  }

  trackpad.addEventListener('touchstart', function(e) {
    e.preventDefault();
    kbInput.blur();
    fingerCount = e.touches.length;
    const t = e.touches[0];
    lastX = t.clientX; lastY = t.clientY;
    tapX = t.clientX; tapY = t.clientY;
    tpActive = true; moved = false;
    tapStart = Date.now();
    trackpad.classList.add('active');
  }, { passive: false });

  trackpad.addEventListener('touchmove', function(e) {
    e.preventDefault();
    if (!tpActive) return;
    const now = Date.now();
    if (now - lastSend < THROTTLE_MS) return;

    fingerCount = e.touches.length;

    if (fingerCount === 2) {
      // Two-finger scroll
      const t = e.touches[0];
      const dy = t.clientY - lastY;
      if (Math.abs(dy) > 2) {
        const scrollDelta = dy > 0 ? -1 : 1;
        post('/api/mouse', { dx: 0, dy: 0, action: 'scroll', delta: scrollDelta });
      }
      lastX = t.clientX; lastY = t.clientY;
    } else {
      const t = e.touches[0];
      const dx = t.clientX - lastX;
      const dy = t.clientY - lastY;
      if (Math.abs(dx) > 1 || Math.abs(dy) > 1) {
        moved = true;
        post('/api/mouse', { dx: Math.round(dx * 1.5), dy: Math.round(dy * 1.5), action: 'move' });
        lastX = t.clientX; lastY = t.clientY;
      }
    }
    lastSend = now;
  }, { passive: false });

  trackpad.addEventListener('touchend', function(e) {
    e.preventDefault();
    trackpad.classList.remove('active');
    const elapsed = Date.now() - tapStart;

    if (!moved && elapsed < 300) {
      const rect = trackpad.getBoundingClientRect();
      if (fingerCount >= 2) {
        post('/api/mouse', { dx: 0, dy: 0, action: 'rightclick' });
        addRipple(tapX - rect.left, tapY - rect.top);
        showToast('Right Click');
      } else {
        post('/api/mouse', { dx: 0, dy: 0, action: 'click' });
        addRipple(tapX - rect.left, tapY - rect.top);
        showToast('Click');
      }
    }

    if (e.touches.length === 0) {
      tpActive = false;
      fingerCount = 0;
    }
  }, { passive: false });

  // Mouse fallback for desktop testing
  trackpad.addEventListener('mousedown', function(e) {
    kbInput.blur();
    lastX = e.clientX; lastY = e.clientY;
    tpActive = true; moved = false;
    tapStart = Date.now();
    tapX = e.clientX; tapY = e.clientY;
    trackpad.classList.add('active');
  });
  document.addEventListener('mousemove', function(e) {
    if (!tpActive) return;
    const now = Date.now();
    if (now - lastSend < THROTTLE_MS) return;
    const dx = e.clientX - lastX;
    const dy = e.clientY - lastY;
    if (Math.abs(dx) > 1 || Math.abs(dy) > 1) {
      moved = true;
      post('/api/mouse', { dx: Math.round(dx * 1.5), dy: Math.round(dy * 1.5), action: 'move' });
      lastX = e.clientX; lastY = e.clientY;
    }
    lastSend = now;
  });
  document.addEventListener('mouseup', function(e) {
    if (!tpActive) return;
    trackpad.classList.remove('active');
    tpActive = false;
    const elapsed = Date.now() - tapStart;
    if (!moved && elapsed < 300) {
      const rect = trackpad.getBoundingClientRect();
      post('/api/mouse', { dx: 0, dy: 0, action: 'click' });
      addRipple(tapX - rect.left, tapY - rect.top);
      showToast('Click');
    }
  });

  // ---- MOUSE BUTTONS ----
  document.getElementById('btnLeft').addEventListener('click', function() {
    kbInput.blur();
    post('/api/mouse', { dx: 0, dy: 0, action: 'click' });
    showToast('Left Click');
  });
  document.getElementById('btnRight').addEventListener('click', function() {
    kbInput.blur();
    post('/api/mouse', { dx: 0, dy: 0, action: 'rightclick' });
    showToast('Right Click');
  });
  document.getElementById('btnMiddle').addEventListener('click', function() {
    kbInput.blur();
    post('/api/mouse', { dx: 0, dy: 0, action: 'middleclick' });
    showToast('Middle Click');
  });

  // ---- SCROLL STRIP ----
  const scrollStrip = document.getElementById('scrollStrip');
  let scrollActive = false, scrollLastY = 0;

  scrollStrip.addEventListener('touchstart', function(e) {
    e.preventDefault();
    kbInput.blur();
    scrollActive = true;
    scrollLastY = e.touches[0].clientY;
    scrollStrip.classList.add('active');
  }, { passive: false });

  scrollStrip.addEventListener('touchmove', function(e) {
    e.preventDefault();
    if (!scrollActive) return;
    const y = e.touches[0].clientY;
    const dy = y - scrollLastY;
    if (Math.abs(dy) > 5) {
      const delta = dy > 0 ? -1 : 1;
      post('/api/mouse', { dx: 0, dy: 0, action: 'scroll', delta: delta });
      scrollLastY = y;
    }
  }, { passive: false });

  scrollStrip.addEventListener('touchend', function() {
    scrollActive = false;
    scrollStrip.classList.remove('active');
  });

  // ---- KEYBOARD INPUT ----

  kbInput.addEventListener('input', function(e) {
    const text = kbInput.value;
    if (text.length > 0) {
      post('/api/keyboard', { text: text });
      kbInput.value = '';
    }
  });

  kbInput.addEventListener('keydown', function(e) {
    // Intercept special keys that browsers handle
    const key = e.key;
    if (['Enter', 'Tab', 'Backspace', 'Escape', 'Delete',
         'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(key)) {
      e.preventDefault();
      const keyMap = {
        'Enter': 'enter', 'Tab': 'tab', 'Backspace': 'backspace',
        'Escape': 'escape', 'Delete': 'delete',
        'ArrowUp': 'up', 'ArrowDown': 'down',
        'ArrowLeft': 'left', 'ArrowRight': 'right'
      };
      post('/api/keyboard', { key: keyMap[key] || key.toLowerCase() });
    }
  });

  // ---- SPECIAL KEYS ----
  document.querySelectorAll('.skey').forEach(function(btn) {
    btn.addEventListener('click', function() {
      const key = btn.getAttribute('data-key');
      post('/api/keyboard', { key: key });
      showToast(key.toUpperCase());
    });
  });
})();
</script>
</body>
</html>
)HTML";


// ==================== Simple JSON Parser ====================
// Minimal JSON value extractor (no external dependency)

static std::string json_get_string(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return "";
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return "";
    size_t end = json.find('"', pos + 1);
    if (end == std::string::npos) return "";
    return json.substr(pos + 1, end - pos - 1);
}

static int json_get_int(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return 0;
    pos = json.find(':', pos + search.size());
    if (pos == std::string::npos) return 0;
    // Skip whitespace
    pos++;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    // Parse integer (handles negative)
    std::string num;
    while (pos < json.size() && (json[pos] == '-' || (json[pos] >= '0' && json[pos] <= '9'))) {
        num += json[pos++];
    }
    if (num.empty()) return 0;
    return std::stoi(num);
}


// ==================== Network Utility ====================

static bool is_private_lan_ip(const std::string& ip) {
    // 192.168.x.x
    if (ip.rfind("192.168.", 0) == 0) return true;
    // 10.x.x.x
    if (ip.rfind("10.", 0) == 0) return true;
    // 172.16.x.x - 172.31.x.x
    if (ip.rfind("172.", 0) == 0) {
        int second = std::atoi(ip.c_str() + 4);
        if (second >= 16 && second <= 31) return true;
    }
    return false;
}

static bool is_link_local(const std::string& ip) {
    return ip.rfind("169.254.", 0) == 0;
}

static std::string get_local_ip() {
#ifdef _WIN32
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) return "127.0.0.1";

    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(hostname, nullptr, &hints, &result) != 0) return "127.0.0.1";

    std::string best_ip = "127.0.0.1";
    std::string fallback_ip;

    for (auto ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        struct sockaddr_in* addr = (struct sockaddr_in*)ptr->ai_addr;
        char buf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
        std::string candidate(buf);

        if (candidate == "127.0.0.1") continue;
        if (is_link_local(candidate)) continue;  // Skip 169.254.x.x

        if (is_private_lan_ip(candidate)) {
            best_ip = candidate;
            break;  // Found a proper LAN IP, use it
        }
        if (fallback_ip.empty()) {
            fallback_ip = candidate;  // Keep first non-loopback, non-link-local
        }
    }
    freeaddrinfo(result);
    return best_ip != "127.0.0.1" ? best_ip : (fallback_ip.empty() ? "127.0.0.1" : fallback_ip);
#else
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) == -1) return "127.0.0.1";

    std::string best_ip = "127.0.0.1";
    std::string fallback_ip;

    for (auto ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;

        char buf[INET_ADDRSTRLEN];
        struct sockaddr_in* addr = (struct sockaddr_in*)ifa->ifa_addr;
        inet_ntop(AF_INET, &addr->sin_addr, buf, sizeof(buf));
        std::string candidate(buf);

        if (candidate == "127.0.0.1") continue;
        if (is_link_local(candidate)) continue;

        if (is_private_lan_ip(candidate)) {
            best_ip = candidate;
            break;
        }
        if (fallback_ip.empty()) {
            fallback_ip = candidate;
        }
    }
    freeifaddrs(ifaddr);
    return best_ip != "127.0.0.1" ? best_ip : (fallback_ip.empty() ? "127.0.0.1" : fallback_ip);
#endif
}


// ==================== Main ====================

int main() {
    std::string ip = get_local_ip();
    int port = 9999;

    std::string url = "http://" + ip + ":" + std::to_string(port);

    std::cout << "\n";
    std::cout << "  ██╗███╗   ██╗██████╗ ██╗   ██╗████████╗" << std::endl;
    std::cout << "  ██║████╗  ██║██╔══██╗██║   ██║╚══██╔══╝" << std::endl;
    std::cout << "  ██║██╔██╗ ██║██████╔╝██║   ██║   ██║   " << std::endl;
    std::cout << "  ██║██║╚██╗██║██╔═══╝ ██║   ██║   ██║   " << std::endl;
    std::cout << "  ██║██║ ╚████║██║     ╚██████╔╝   ██║   " << std::endl;
    std::cout << "  ╚═╝╚═╝  ╚═══╝╚═╝      ╚═════╝    ╚═╝   " << std::endl;
    std::cout << "  ███████╗████████╗██████╗ ███████╗ █████╗ ███╗   ███╗" << std::endl;
    std::cout << "  ██╔════╝╚══██╔══╝██╔══██╗██╔════╝██╔══██╗████╗ ████║" << std::endl;
    std::cout << "  ███████╗   ██║   ██████╔╝█████╗  ███████║██╔████╔██║" << std::endl;
    std::cout << "  ╚════██║   ██║   ██╔══██╗██╔══╝  ██╔══██║██║╚██╔╝██║" << std::endl;
    std::cout << "  ███████║   ██║   ██║  ██║███████╗██║  ██║██║ ╚═╝ ██║" << std::endl;
    std::cout << "  ╚══════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝     ╚═╝" << std::endl;
    std::cout << std::endl;
    std::cout << "  Remote Input Controller v1.0 | By Rhythm113" << std::endl;
    std::cout << "  ──────────────────────────────────────────────" << std::endl;
    std::cout << std::endl;

    // Initialize input injection
    if (!InputInjector::init()) {
        std::cerr << "  [!] ERROR: Failed to initialize input injection." << std::endl;
        std::cerr << "      On Linux : run as root or add user to 'input' group" << std::endl;
        std::cerr << "      On macOS : grant Accessibility permissions" << std::endl;
        return 1;
    }
    std::cout << "  [+] Input injection initialized" << std::endl;

    httplib::Server svr;

    // Serve the web portal
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(HTML_PAGE, "text/html; charset=utf-8");
    });

    // Mouse API
    svr.Post("/api/mouse", [](const httplib::Request& req, httplib::Response& res) {
        const std::string& body = req.body;
        std::string action = json_get_string(body, "action");
        int dx = json_get_int(body, "dx");
        int dy = json_get_int(body, "dy");

        if (action == "move") {
            InputInjector::move_mouse_relative(dx, dy);
        } else if (action == "click") {
            InputInjector::mouse_click(0);
        } else if (action == "rightclick") {
            InputInjector::mouse_click(1);
        } else if (action == "middleclick") {
            InputInjector::mouse_click(2);
        } else if (action == "scroll") {
            int delta = json_get_int(body, "delta");
            InputInjector::mouse_scroll(delta);
        }

        res.set_content("{\"ok\":true}", "application/json");
    });

    // Keyboard API
    svr.Post("/api/keyboard", [](const httplib::Request& req, httplib::Response& res) {
        const std::string& body = req.body;
        std::string text = json_get_string(body, "text");
        std::string key = json_get_string(body, "key");

        if (!text.empty()) {
            InputInjector::type_text(text);
        }
        if (!key.empty()) {
            InputInjector::press_key(key);
        }

        res.set_content("{\"ok\":true}", "application/json");
    });

    // Health check
    svr.Get("/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"ok\"}", "application/json");
    });

    std::cout << "  [+] Server starting on 0.0.0.0:" << port << std::endl;
    std::cout << std::endl;
    std::cout << "  ┌──────────────────────────────────────────┐" << std::endl;
    std::cout << "  │  Open on your phone or browser:          │" << std::endl;
    std::cout << "  │                                          │" << std::endl;
    std::cout << "  │  -> " << url;
    // Pad to align the box
    for (size_t i = url.size(); i < 35; i++) std::cout << " ";
    std::cout << " │" << std::endl;
    std::cout << "  │  -> http://localhost:" << port;
    std::string local_url = "http://localhost:" + std::to_string(port);
    for (size_t i = local_url.size(); i < 35; i++) std::cout << " ";
    std::cout << "  │" << std::endl;
    std::cout << "  │                                          │" << std::endl;
    std::cout << "  └──────────────────────────────────────────┘" << std::endl;
    std::cout << std::endl;
    std::cout << "  [*] Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    if (!svr.listen("0.0.0.0", port)) {
        std::cerr << "  [!] ERROR: Failed to start server on port " << port << std::endl;
        std::cerr << "      Is port " << port << " already in use?" << std::endl;
        InputInjector::shutdown();
        return 1;
    }

    InputInjector::shutdown();
    return 0;
}
