#ifdef _WIN32

#include "input_injector.h"
#include <windows.h>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <cctype>

namespace InputInjector {

static const std::unordered_map<std::string, WORD> SPECIAL_KEYS = {
    {"enter",      VK_RETURN},
    {"tab",        VK_TAB},
    {"backspace",  VK_BACK},
    {"escape",     VK_ESCAPE},
    {"esc",        VK_ESCAPE},
    {"delete",     VK_DELETE},
    {"del",        VK_DELETE},
    {"home",       VK_HOME},
    {"end",        VK_END},
    {"pageup",     VK_PRIOR},
    {"pagedown",   VK_NEXT},
    {"up",         VK_UP},
    {"down",       VK_DOWN},
    {"left",       VK_LEFT},
    {"right",      VK_RIGHT},
    {"space",      VK_SPACE},
    {"insert",     VK_INSERT},
    {"f1",  VK_F1},  {"f2",  VK_F2},  {"f3",  VK_F3},  {"f4",  VK_F4},
    {"f5",  VK_F5},  {"f6",  VK_F6},  {"f7",  VK_F7},  {"f8",  VK_F8},
    {"f9",  VK_F9},  {"f10", VK_F10}, {"f11", VK_F11}, {"f12", VK_F12},
    {"printscreen", VK_SNAPSHOT},
    {"capslock",    VK_CAPITAL},
    {"numlock",     VK_NUMLOCK},
    {"scrolllock",  VK_SCROLL},
    {"win",         VK_LWIN},
    {"meta",        VK_LWIN},
    {"shift",       VK_LSHIFT},
    {"ctrl",        VK_LCONTROL},
    {"control",     VK_LCONTROL},
    {"alt",         VK_LMENU},
};

bool init() {
    return true; // No special init needed on Windows
}

void shutdown() {
    // Nothing to clean up
}

void move_mouse_relative(int dx, int dy) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
}

void mouse_click(int button) {
    mouse_down(button);
    mouse_up(button);
}

void mouse_down(int button) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    switch (button) {
        case 0: input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN; break;
        case 1: input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN; break;
        case 2: input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN; break;
    }
    SendInput(1, &input, sizeof(INPUT));
}

void mouse_up(int button) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    switch (button) {
        case 0: input.mi.dwFlags = MOUSEEVENTF_LEFTUP; break;
        case 1: input.mi.dwFlags = MOUSEEVENTF_RIGHTUP; break;
        case 2: input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP; break;
    }
    SendInput(1, &input, sizeof(INPUT));
}

void mouse_scroll(int delta) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    input.mi.mouseData = delta * WHEEL_DELTA;
    SendInput(1, &input, sizeof(INPUT));
}

static void send_key(WORD vk, bool down) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    if (!down) input.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

static void send_unicode_char(wchar_t ch) {
    INPUT inputs[2] = {};
    // Key down
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wScan = ch;
    inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
    // Key up
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wScan = ch;
    inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

void type_text(const std::string& text) {
    // Convert UTF-8 to wide string
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), nullptr, 0);
    if (wlen <= 0) return;
    std::wstring wtext(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), &wtext[0], wlen);

    for (wchar_t ch : wtext) {
        send_unicode_char(ch);
    }
}

static std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

void press_key(const std::string& key) {
    std::string k = to_lower(key);

    // Handle modifier combos like "ctrl+c", "ctrl+shift+v", "alt+tab"
    std::vector<WORD> modifiers;
    std::string main_key = k;

    // Split by '+'
    size_t pos;
    while ((pos = main_key.find('+')) != std::string::npos) {
        std::string mod = main_key.substr(0, pos);
        main_key = main_key.substr(pos + 1);

        if (mod == "ctrl" || mod == "control") modifiers.push_back(VK_LCONTROL);
        else if (mod == "shift")               modifiers.push_back(VK_LSHIFT);
        else if (mod == "alt")                 modifiers.push_back(VK_LMENU);
        else if (mod == "win" || mod == "meta") modifiers.push_back(VK_LWIN);
    }

    // Press modifiers
    for (WORD mod : modifiers) send_key(mod, true);

    // Press the main key
    auto it = SPECIAL_KEYS.find(main_key);
    if (it != SPECIAL_KEYS.end()) {
        send_key(it->second, true);
        send_key(it->second, false);
    } else if (main_key.size() == 1) {
        // Single character — use VkKeyScan
        SHORT vk = VkKeyScanA(main_key[0]);
        if (vk != -1) {
            send_key(LOBYTE(vk), true);
            send_key(LOBYTE(vk), false);
        }
    }

    // Release modifiers (reverse order)
    for (auto rit = modifiers.rbegin(); rit != modifiers.rend(); ++rit) {
        send_key(*rit, false);
    }
}

} // namespace InputInjector

#endif // _WIN32
