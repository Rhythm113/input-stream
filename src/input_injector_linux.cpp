#ifdef __linux__

#include "input_injector.h"
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <vector>

namespace InputInjector {

static int uinput_fd = -1;

static const std::unordered_map<std::string, int> SPECIAL_KEYS = {
    {"enter",      KEY_ENTER},
    {"tab",        KEY_TAB},
    {"backspace",  KEY_BACKSPACE},
    {"escape",     KEY_ESC},
    {"esc",        KEY_ESC},
    {"delete",     KEY_DELETE},
    {"del",        KEY_DELETE},
    {"home",       KEY_HOME},
    {"end",        KEY_END},
    {"pageup",     KEY_PAGEUP},
    {"pagedown",   KEY_PAGEDOWN},
    {"up",         KEY_UP},
    {"down",       KEY_DOWN},
    {"left",       KEY_LEFT},
    {"right",      KEY_RIGHT},
    {"space",      KEY_SPACE},
    {"insert",     KEY_INSERT},
    {"f1",  KEY_F1},  {"f2",  KEY_F2},  {"f3",  KEY_F3},  {"f4",  KEY_F4},
    {"f5",  KEY_F5},  {"f6",  KEY_F6},  {"f7",  KEY_F7},  {"f8",  KEY_F8},
    {"f9",  KEY_F9},  {"f10", KEY_F10}, {"f11", KEY_F11}, {"f12", KEY_F12},
    {"capslock",    KEY_CAPSLOCK},
    {"numlock",     KEY_NUMLOCK},
    {"scrolllock",  KEY_SCROLLLOCK},
    {"shift",       KEY_LEFTSHIFT},
    {"ctrl",        KEY_LEFTCTRL},
    {"control",     KEY_LEFTCTRL},
    {"alt",         KEY_LEFTALT},
    {"meta",        KEY_LEFTMETA},
    {"win",         KEY_LEFTMETA},
};

// Map ASCII chars to Linux keycodes
static const std::unordered_map<char, int> CHAR_KEYS = {
    {'a', KEY_A}, {'b', KEY_B}, {'c', KEY_C}, {'d', KEY_D}, {'e', KEY_E},
    {'f', KEY_F}, {'g', KEY_G}, {'h', KEY_H}, {'i', KEY_I}, {'j', KEY_J},
    {'k', KEY_K}, {'l', KEY_L}, {'m', KEY_M}, {'n', KEY_N}, {'o', KEY_O},
    {'p', KEY_P}, {'q', KEY_Q}, {'r', KEY_R}, {'s', KEY_S}, {'t', KEY_T},
    {'u', KEY_U}, {'v', KEY_V}, {'w', KEY_W}, {'x', KEY_X}, {'y', KEY_Y},
    {'z', KEY_Z},
    {'1', KEY_1}, {'2', KEY_2}, {'3', KEY_3}, {'4', KEY_4}, {'5', KEY_5},
    {'6', KEY_6}, {'7', KEY_7}, {'8', KEY_8}, {'9', KEY_9}, {'0', KEY_0},
    {' ', KEY_SPACE}, {'\n', KEY_ENTER}, {'\t', KEY_TAB},
    {'-', KEY_MINUS}, {'=', KEY_EQUAL}, {'[', KEY_LEFTBRACE},
    {']', KEY_RIGHTBRACE}, {'\\', KEY_BACKSLASH}, {';', KEY_SEMICOLON},
    {'\'', KEY_APOSTROPHE}, {'`', KEY_GRAVE}, {',', KEY_COMMA},
    {'.', KEY_DOT}, {'/', KEY_SLASH},
};

static void emit_event(int type, int code, int val) {
    struct input_event ev = {};
    ev.type = type;
    ev.code = code;
    ev.value = val;
    write(uinput_fd, &ev, sizeof(ev));
}

static void sync() {
    emit_event(EV_SYN, SYN_REPORT, 0);
}

bool init() {
    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) return false;

    // Enable mouse events
    ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_WHEEL);

    // Enable key events
    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_MIDDLE);

    // Enable all keyboard keys
    for (int i = 0; i < 256; i++) {
        ioctl(uinput_fd, UI_SET_KEYBIT, i);
    }

    struct uinput_setup usetup = {};
    std::strncpy(usetup.name, "InputStream Virtual Device", UINPUT_MAX_NAME_SIZE);
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;

    ioctl(uinput_fd, UI_DEV_SETUP, &usetup);
    ioctl(uinput_fd, UI_DEV_CREATE);

    // Small delay to let the device register
    usleep(100000);
    return true;
}

void shutdown() {
    if (uinput_fd >= 0) {
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
        uinput_fd = -1;
    }
}

void move_mouse_relative(int dx, int dy) {
    emit_event(EV_REL, REL_X, dx);
    emit_event(EV_REL, REL_Y, dy);
    sync();
}

static int button_code(int button) {
    switch (button) {
        case 0: return BTN_LEFT;
        case 1: return BTN_RIGHT;
        case 2: return BTN_MIDDLE;
        default: return BTN_LEFT;
    }
}

void mouse_click(int button) {
    mouse_down(button);
    mouse_up(button);
}

void mouse_down(int button) {
    emit_event(EV_KEY, button_code(button), 1);
    sync();
}

void mouse_up(int button) {
    emit_event(EV_KEY, button_code(button), 0);
    sync();
}

void mouse_scroll(int delta) {
    emit_event(EV_REL, REL_WHEEL, delta);
    sync();
}

void type_text(const std::string& text) {
    for (char ch : text) {
        char lower = std::tolower(ch);
        auto it = CHAR_KEYS.find(lower);
        if (it != CHAR_KEYS.end()) {
            bool need_shift = std::isupper(ch);
            if (need_shift) {
                emit_event(EV_KEY, KEY_LEFTSHIFT, 1);
                sync();
            }
            emit_event(EV_KEY, it->second, 1);
            sync();
            emit_event(EV_KEY, it->second, 0);
            sync();
            if (need_shift) {
                emit_event(EV_KEY, KEY_LEFTSHIFT, 0);
                sync();
            }
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

    std::vector<int> modifiers;
    std::string main_key = k;

    size_t pos;
    while ((pos = main_key.find('+')) != std::string::npos) {
        std::string mod = main_key.substr(0, pos);
        main_key = main_key.substr(pos + 1);

        if (mod == "ctrl" || mod == "control") modifiers.push_back(KEY_LEFTCTRL);
        else if (mod == "shift")               modifiers.push_back(KEY_LEFTSHIFT);
        else if (mod == "alt")                 modifiers.push_back(KEY_LEFTALT);
        else if (mod == "win" || mod == "meta") modifiers.push_back(KEY_LEFTMETA);
    }

    for (int mod : modifiers) {
        emit_event(EV_KEY, mod, 1);
        sync();
    }

    auto it = SPECIAL_KEYS.find(main_key);
    if (it != SPECIAL_KEYS.end()) {
        emit_event(EV_KEY, it->second, 1);
        sync();
        emit_event(EV_KEY, it->second, 0);
        sync();
    } else if (main_key.size() == 1) {
        auto cit = CHAR_KEYS.find(main_key[0]);
        if (cit != CHAR_KEYS.end()) {
            emit_event(EV_KEY, cit->second, 1);
            sync();
            emit_event(EV_KEY, cit->second, 0);
            sync();
        }
    }

    for (auto rit = modifiers.rbegin(); rit != modifiers.rend(); ++rit) {
        emit_event(EV_KEY, *rit, 0);
        sync();
    }
}

} // namespace InputInjector

#endif // __linux__
