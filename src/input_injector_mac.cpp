#ifdef __APPLE__

#include "input_injector.h"
#include <ApplicationServices/ApplicationServices.h>
#include <unordered_map>
#include <algorithm>
#include <vector>

namespace InputInjector {

static CGPoint current_mouse_pos = {0, 0};

// macOS virtual keycodes
static const std::unordered_map<std::string, CGKeyCode> SPECIAL_KEYS = {
    {"enter",      36}, {"tab",        48}, {"backspace",  51},
    {"escape",     53}, {"esc",        53}, {"delete",     117},
    {"del",        117}, {"home",      115}, {"end",        119},
    {"pageup",     116}, {"pagedown",  121},
    {"up",         126}, {"down",      125}, {"left",       123}, {"right",     124},
    {"space",      49},  {"insert",    114},
    {"f1", 122}, {"f2", 120}, {"f3", 99},  {"f4", 118},
    {"f5", 96},  {"f6", 97},  {"f7", 98},  {"f8", 100},
    {"f9", 101}, {"f10", 109}, {"f11", 103}, {"f12", 111},
    {"capslock",   57}, {"shift",     56}, {"ctrl",       59},
    {"control",    59}, {"alt",       58}, {"meta",       55},
    {"win",        55}, {"command",   55},
};

static const std::unordered_map<char, CGKeyCode> CHAR_KEYS = {
    {'a', 0},  {'b', 11}, {'c', 8},  {'d', 2},  {'e', 14}, {'f', 3},
    {'g', 5},  {'h', 4},  {'i', 34}, {'j', 38}, {'k', 40}, {'l', 37},
    {'m', 46}, {'n', 45}, {'o', 31}, {'p', 35}, {'q', 12}, {'r', 15},
    {'s', 1},  {'t', 17}, {'u', 32}, {'v', 9},  {'w', 13}, {'x', 7},
    {'y', 16}, {'z', 6},
    {'1', 18}, {'2', 19}, {'3', 20}, {'4', 21}, {'5', 23},
    {'6', 22}, {'7', 26}, {'8', 28}, {'9', 25}, {'0', 29},
    {' ', 49}, {'-', 27}, {'=', 24}, {'[', 33}, {']', 30},
    {'\\', 42}, {';', 41}, {'\'', 39}, {'`', 50}, {',', 43},
    {'.', 47}, {'/', 44},
};

bool init() {
    // Get current mouse position
    CGEventRef event = CGEventCreate(NULL);
    current_mouse_pos = CGEventGetLocation(event);
    CFRelease(event);
    return true;
}

void shutdown() {
    // Nothing to clean up on macOS
}

void move_mouse_relative(int dx, int dy) {
    current_mouse_pos.x += dx;
    current_mouse_pos.y += dy;

    // Clamp to screen bounds
    CGDirectDisplayID display = CGMainDisplayID();
    size_t w = CGDisplayPixelsWide(display);
    size_t h = CGDisplayPixelsHigh(display);
    if (current_mouse_pos.x < 0) current_mouse_pos.x = 0;
    if (current_mouse_pos.y < 0) current_mouse_pos.y = 0;
    if (current_mouse_pos.x >= w) current_mouse_pos.x = w - 1;
    if (current_mouse_pos.y >= h) current_mouse_pos.y = h - 1;

    CGEventRef move = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved,
                                               current_mouse_pos, kCGMouseButtonLeft);
    CGEventPost(kCGHIDEventTap, move);
    CFRelease(move);
}

static CGMouseButton to_cg_button(int button) {
    switch (button) {
        case 0: return kCGMouseButtonLeft;
        case 1: return kCGMouseButtonRight;
        case 2: return kCGMouseButtonCenter;
        default: return kCGMouseButtonLeft;
    }
}

static CGEventType down_event(int button) {
    switch (button) {
        case 0: return kCGEventLeftMouseDown;
        case 1: return kCGEventRightMouseDown;
        case 2: return kCGEventOtherMouseDown;
        default: return kCGEventLeftMouseDown;
    }
}

static CGEventType up_event(int button) {
    switch (button) {
        case 0: return kCGEventLeftMouseUp;
        case 1: return kCGEventRightMouseUp;
        case 2: return kCGEventOtherMouseUp;
        default: return kCGEventLeftMouseUp;
    }
}

void mouse_click(int button) {
    mouse_down(button);
    mouse_up(button);
}

void mouse_down(int button) {
    CGEventRef ev = CGEventCreateMouseEvent(NULL, down_event(button),
                                             current_mouse_pos, to_cg_button(button));
    CGEventPost(kCGHIDEventTap, ev);
    CFRelease(ev);
}

void mouse_up(int button) {
    CGEventRef ev = CGEventCreateMouseEvent(NULL, up_event(button),
                                             current_mouse_pos, to_cg_button(button));
    CGEventPost(kCGHIDEventTap, ev);
    CFRelease(ev);
}

void mouse_scroll(int delta) {
    CGEventRef ev = CGEventCreateScrollWheelEvent(NULL, kCGScrollEventUnitLine, 1, delta);
    CGEventPost(kCGHIDEventTap, ev);
    CFRelease(ev);
}

static void send_key_event(CGKeyCode keycode, bool down) {
    CGEventRef ev = CGEventCreateKeyboardEvent(NULL, keycode, down);
    CGEventPost(kCGHIDEventTap, ev);
    CFRelease(ev);
}

void type_text(const std::string& text) {
    for (char ch : text) {
        char lower = std::tolower(ch);
        auto it = CHAR_KEYS.find(lower);
        if (it != CHAR_KEYS.end()) {
            bool need_shift = std::isupper(ch);
            if (need_shift) send_key_event(56, true); // shift down
            send_key_event(it->second, true);
            send_key_event(it->second, false);
            if (need_shift) send_key_event(56, false); // shift up
        }
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

    std::vector<CGKeyCode> modifiers;
    std::string main_key = k;

    size_t pos;
    while ((pos = main_key.find('+')) != std::string::npos) {
        std::string mod = main_key.substr(0, pos);
        main_key = main_key.substr(pos + 1);

        if (mod == "ctrl" || mod == "control") modifiers.push_back(59);
        else if (mod == "shift")               modifiers.push_back(56);
        else if (mod == "alt")                 modifiers.push_back(58);
        else if (mod == "win" || mod == "meta" || mod == "command") modifiers.push_back(55);
    }

    for (CGKeyCode mod : modifiers) send_key_event(mod, true);

    auto it = SPECIAL_KEYS.find(main_key);
    if (it != SPECIAL_KEYS.end()) {
        send_key_event(it->second, true);
        send_key_event(it->second, false);
    } else if (main_key.size() == 1) {
        auto cit = CHAR_KEYS.find(main_key[0]);
        if (cit != CHAR_KEYS.end()) {
            send_key_event(cit->second, true);
            send_key_event(cit->second, false);
        }
    }

    for (auto rit = modifiers.rbegin(); rit != modifiers.rend(); ++rit) {
        send_key_event(*rit, false);
    }
}

} // namespace InputInjector

#endif // __APPLE__
