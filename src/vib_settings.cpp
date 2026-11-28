#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include "psx_menu_host.h"
#include "psx_host_menu.h"
#include "psx_sdl.h"
#include "internal_resolution.h"
#include "mod_runtime.h"
#include "mod_plugins.h"
#include "psx_memory.h"
#include "psx_disc_tray.h"
extern "C" uint32_t psx_read_word(uint32_t addr);

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <future>
#include <cmath>
#include <mutex>
#include <string>
#include <vector>

namespace {

constexpr int kMaxMusicSeconds = 600;
constexpr uint32_t kSectorFrames = 588;
constexpr uint32_t kSectorBytes = 2352;
constexpr int kUiW = 640;
constexpr int kUiH = 480;
constexpr int kRows = 9;
constexpr int kRowH = 42;

const uint8_t kFont[59][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00},
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00},
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00},
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00},
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00},
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00},
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00},
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00},
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00},
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00},
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00},
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06},
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00}, {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00},
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00},
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00}, {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00},
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00},
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, {0x7F,0x06,0x06,0x3E,0x06,0x06,0x7F,0x00},
    {0x7F,0x06,0x06,0x3E,0x06,0x06,0x06,0x00}, {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00},
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00},
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00}, {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00},
    {0x06,0x06,0x06,0x06,0x06,0x06,0x7F,0x00}, {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00},
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00},
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x06,0x00}, {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00},
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00}, {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00},
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00},
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00},
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00},
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00},
};

struct ControllerEntry {
    std::wstring name;
    std::string guid;
};

struct Settings {
    std::string resolution = "auto";
    int fullscreen = 0;
    std::string controller = "auto";
    int offset_ms = 0;
    std::vector<std::wstring> music;
};

enum UiMode { kClosed = 0, kSettings = 1, kMusic = 2 };

const PsxMenuHost* g_host = nullptr;
std::mutex g_ui_mutex;
UiMode g_ui_mode = kClosed;
bool g_ui_open = false;
int g_selection = 0;
int g_scroll = 0;
int g_music_selection = -1;
std::string g_status;
Settings g_settings;
std::vector<std::wstring> g_entries;
std::vector<ControllerEntry> g_controllers;
std::wstring g_exe_dir;
std::wstring g_pick_dir;
std::vector<std::vector<uint8_t>> g_music_sectors;
std::atomic<bool> g_want_relaunch{false};
bool g_want_pick_music = false;
bool g_cd_request_seen = false;
struct PreparedMusic {
    std::vector<std::vector<uint8_t>> tracks;
    std::string error;
};
std::future<PreparedMusic> g_decode;
bool g_decode_cancelled = false;

// PAL SCES-02873 MAIN_G.EXE: checked against its instructions and live
// captures of the own-CD submenu and the Insert a music CD screen.
bool game_module_loaded() {
    return psx_read_word(0x80037900u) == 0x3C028007u &&
           psx_read_word(0x80037904u) == 0x8C4263C4u;
}
bool music_cd_requested() {
    return game_module_loaded() &&
           psx_read_word(0x800763C4u) == 8 &&
           psx_read_word(0x800763CCu) == 8 &&
           psx_read_word(0x80076284u) == 4;
}
std::chrono::steady_clock::time_point g_last_axis_nav;

const char* const kResValues[] = {"auto", "native", "2x", "3x", "4x", "6x", "8x"};
const char* const kResLabels[] = {"AUTO (TELA)", "NATIVO 1X", "2X", "3X", "4X", "6X", "8X"};
const char* const kFullLabels[] = {"JANELA", "TELA INTEIRA", "EXCLUSIVA"};

std::string trim(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    const size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::wstring utf8_to_wide(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return {};
    std::wstring out((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), n);
    return out;
}

std::string wide_to_utf8(const std::wstring& s) {
    if (s.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return {};
    std::string out((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), out.data(), n, nullptr, nullptr);
    return out;
}

std::string ascii(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
        out.push_back(c >= 32 && c <= 126 ? (char)c : '?');
    return out;
}

std::string fit(const std::string& s, size_t max_chars) {
    const std::string clean = ascii(s);
    return clean.size() <= max_chars ? clean : clean.substr(0, max_chars);
}

std::wstring config_path() {
    return g_exe_dir + L"vib_settings.ini";
}

std::wstring resolve_path(const std::wstring& p) {
    if (p.empty()) return {};
    if (p[0] == L'\\' || (p.size() > 1 && p[1] == L':')) return p;
    return g_exe_dir + p;
}

int parse_int(const std::string& s, int fallback) {
    try {
        size_t used = 0;
        const int value = std::stoi(trim(s), &used);
        return used == trim(s).size() ? value : fallback;
    } catch (...) {
        return fallback;
    }
}

Settings load_settings() {
    Settings s;
    std::ifstream in{std::filesystem::path(config_path())};
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#') continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = trim(line.substr(0, eq));
        const std::string value = trim(line.substr(eq + 1));
        if (key == "resolution") s.resolution = value;
        else if (key == "fullscreen") s.fullscreen = parse_int(value, 0);
        else if (key == "controller") s.controller = value;
        else if (key == "offset_ms") s.offset_ms = parse_int(value, 0);
        else if (key.rfind("music", 0) == 0 && !value.empty()) s.music.push_back(utf8_to_wide(value));
    }
    s.offset_ms = std::max(-2000, std::min(2000, s.offset_ms));
    return s;
}

void save_settings(const Settings& s) {
    std::ofstream out(std::filesystem::path(config_path()), std::ios::trunc);
    out << "; Vib-Ribbon PC - F1 para editar\n";
    out << "resolution=" << s.resolution << "\n";
    out << "fullscreen=" << s.fullscreen << "\n";
    out << "controller=" << s.controller << "\n";
    out << "offset_ms=" << s.offset_ms << "\n";
    for (size_t i = 0; i < s.music.size(); ++i)
        out << "music" << (i + 1) << "=" << wide_to_utf8(s.music[i]) << "\n";
}

bool read_file_w(const std::wstring& path, std::vector<uint8_t>& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    if (!GetFileSizeEx(h, &size) || size.QuadPart < 0 || size.QuadPart > 512ull * 1024 * 1024) {
        CloseHandle(h);
        return false;
    }
    out.resize((size_t)size.QuadPart);
    DWORD got = 0;
    const bool ok = !out.empty()
        ? ReadFile(h, out.data(), (DWORD)out.size(), &got, nullptr) && got == out.size()
        : true;
    CloseHandle(h);
    if (!ok) out.clear();
    return ok;
}

bool decode_mp3(const std::wstring& path, int offset_ms, std::vector<uint8_t>& sectors, std::string& error) {
    std::vector<uint8_t> bytes;
    if (!read_file_w(path, bytes) || bytes.empty()) {
        error = "nao foi possivel ler o arquivo";
        return false;
    }
    drmp3 decoder{};
    if (!drmp3_init_memory(&decoder, bytes.data(), bytes.size(), nullptr)) {
        error = "MP3 invalido ou sem audio";
        return false;
    }
    // dr_mp3 reports the source format; assigning drmp3_config never
    // requests conversion. Bound decoding before allocating, including mono.
    const unsigned channels = decoder.channels, rate = decoder.sampleRate;
    if ((channels != 1 && channels != 2) || rate == 0) {
        drmp3_uninit(&decoder);
        error = "formato MP3 nao suportado";
        return false;
    }
    const size_t limit = (size_t)(kMaxMusicSeconds + 2) * rate;
    std::vector<int16_t> pcm(limit * channels);
    const size_t total = (size_t)drmp3_read_pcm_frames_s16(&decoder, limit, pcm.data());
    drmp3_uninit(&decoder);
    if (!total) { error = "MP3 sem audio"; return false; }
    const int64_t shift = -(int64_t)offset_ms * 44100 / 1000;
    const size_t prefix = shift > 0 ? (size_t)shift : 0;
    const size_t skip = shift < 0 ? (size_t)-shift : 0;
    const size_t converted = (uint64_t)total * 44100 / rate;
    const size_t usable = std::min(converted > skip ? converted - skip : 0,
                                  (size_t)kMaxMusicSeconds * 44100 - prefix);
    if (!usable) { error = "MP3 sem audio apos o offset"; return false; }
    const size_t frames = prefix + usable;
    sectors.assign(((frames + kSectorFrames - 1) / kSectorFrames) * kSectorBytes, 0);
    for (size_t i = 0; i < usable; ++i) {
        const double pos = (double)(skip + i) * rate / 44100.0;
        const size_t left = std::min((size_t)pos, total - 1);
        const size_t right = std::min(left + 1, total - 1);
        const double fraction = pos - std::floor(pos);
        for (unsigned ch = 0; ch < 2; ++ch) {
            const unsigned source_ch = channels == 1 ? 0 : ch;
            const int16_t sample = (int16_t)std::lround(
                pcm[left * channels + source_ch] * (1.0 - fraction) +
                pcm[right * channels + source_ch] * fraction);
            const size_t dst = (prefix + i) * 4 + ch * 2;
            sectors[dst] = (uint8_t)sample;
            sectors[dst + 1] = (uint8_t)((uint16_t)sample >> 8);
        }
    }
    return true;
}

PreparedMusic prepare_music(const Settings& settings) {
    PreparedMusic result;
    // A physical audio CD is bounded; avoid multi-gigabyte playlists.
    uint64_t sectors_total = 0;
    for (const auto& entry : settings.music) {
        std::vector<uint8_t> sectors;
        if (!decode_mp3(entry, settings.offset_ms, sectors, result.error)) {
            result.tracks.clear();
            return result;
        }
        sectors_total += sectors.size() / kSectorBytes;
        if (sectors_total > 80u * 60u * 75u || result.tracks.size() >= 98) {
            result.error = "LIMITE: 80 MINUTOS E 98 FAIXAS";
            result.tracks.clear();
            return result;
        }
        result.tracks.push_back(std::move(sectors));
    }
    return result;
}

void begin_music_decode() {
    if (g_decode.valid()) return;
    Settings settings = g_settings;
    for (auto& path : settings.music) path = resolve_path(path);
    g_decode_cancelled = false;
    g_status = "PREPARANDO AUDIO... AGUARDE";
    g_decode = std::async(std::launch::async, [settings] {
        try { return prepare_music(settings); }
        catch (const std::exception&) {
            PreparedMusic failed;
            failed.error = "SEM MEMORIA PARA PREPARAR O AUDIO";
            return failed;
        }
    });
}

int resolution_value(const std::string& name) {
    if (name == "native") return PSX_IR_NATIVE;
    if (name == "2x") return PSX_IR_SCALE(2);
    if (name == "3x") return PSX_IR_SCALE(3);
    if (name == "4x") return PSX_IR_SCALE(4);
    if (name == "6x") return PSX_IR_SCALE(6);
    if (name == "8x") return PSX_IR_SCALE(8);
    return PSX_IR_AUTO;
}

bool apply_video(const Settings& s) {
    if (!g_host || !g_host->video_get || !g_host->video_apply) return false;
    PsxHostVideoSettings video{};
    g_host->video_get(&video);
    const int wanted = resolution_value(s.resolution);
    const int fullscreen = std::max(0, std::min(2, s.fullscreen));
    if (video.internal_resolution == wanted && video.fullscreen == fullscreen) return false;
    video.internal_resolution = wanted;
    video.fullscreen = fullscreen;
    g_host->video_apply(&video);
    return true;
}

void set_exe_dir() {
    const char* base = SDL_GetBasePath();
    if (base) {
        g_exe_dir = utf8_to_wide(base);
        SDL_free((void*)base);
    }
    if (g_exe_dir.empty()) g_exe_dir = L"./";
    g_pick_dir = g_exe_dir;
}

void refresh_controllers() {
    g_controllers.clear();
    const int joysticks = SDL_NumJoysticks();
    for (int i = 0; i < joysticks; ++i) {
        const SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(i);
        if (id < 0) continue;
        char guid[64]{};
        const SDL_JoystickGUID g = SDL_GetJoystickGUIDForID(id);
        SDL_JoystickGetGUIDString(g, guid, sizeof(guid));
        const char* name = SDL_GetJoystickNameForID(id);
        ControllerEntry entry;
        entry.guid = guid;
        entry.name = utf8_to_wide(name ? name : "Controle USB");
        entry.name += L" [" + utf8_to_wide(guid) + L"]";
        g_controllers.push_back(std::move(entry));
    }
}

void fill_rect(uint32_t* dst, int x, int y, int w, int h, uint32_t col) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > kUiW) w = kUiW - x;
    if (y + h > kUiH) h = kUiH - y;
    if (w <= 0 || h <= 0) return;
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            dst[yy * kUiW + xx] = col;
}

void stroke_rect(uint32_t* dst, int x, int y, int w, int h, uint32_t col) {
    fill_rect(dst, x, y, w, 1, col);
    fill_rect(dst, x, y + h - 1, w, 1, col);
    fill_rect(dst, x, y, 1, h, col);
    fill_rect(dst, x + w - 1, y, 1, h, col);
}

void draw_char(uint32_t* dst, int x0, int y0, char c, uint32_t col, int scale) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    if (c < 32 || c > 90) c = '?';
    const uint8_t* glyph = kFont[(unsigned char)c - 32];
    for (int y = 0; y < 8; ++y) {
        const uint8_t row = glyph[y];
        for (int x = 0; x < 8; ++x) {
            if ((row & (1u << x)) == 0) continue;
            for (int sy = 0; sy < scale; ++sy)
                for (int sx = 0; sx < scale; ++sx)
                    fill_rect(dst, x0 + x * scale + sx, y0 + y * scale + sy, 1, 1, col);
        }
    }
}

void draw_text(uint32_t* dst, int x, int y, const std::string& s, uint32_t col, int scale) {
    for (char c : s) {
        if (x + 8 * scale > kUiW) break;
        draw_char(dst, x, y, c, col, scale);
        x += 8 * scale;
    }
}

void draw_row(uint32_t* dst, int index, int row, const std::string& text) {
    const int y = 76 + row * kRowH;
    const bool selected = index == row;
    fill_rect(dst, 20, y, 600, kRowH - 4, selected ? 0xFF252B3A : 0xFF141821);
    stroke_rect(dst, 20, y, 600, kRowH - 4, selected ? 0xFFFFD24D : 0xFF2B3242);
    draw_text(dst, 34, y + 9, fit(text, 34), selected ? 0xFFFFD24D : 0xFFDDE2EA, 2);
}

std::string resolution_label(const Settings& s) {
    for (size_t i = 0; i < 7; ++i)
        if (s.resolution == kResValues[i]) return kResLabels[i];
    return kResLabels[0];
}

std::string controller_label(const Settings& s) {
    if (s.controller == "auto") return "AUTO (PRIMEIRO CONTROLE)";
    for (const auto& entry : g_controllers)
        if (entry.guid == s.controller)
            return fit(wide_to_utf8(entry.name), 26);
    return "AUTO (CONTROLE NAO ENCONTRADO)";
}

std::string file_label(const std::wstring& path) {
    std::wstring name = std::filesystem::path(path).filename().wstring();
    if (name.empty()) name = path;
    return fit(wide_to_utf8(name), 30);
}

int row_count() {
    if (g_ui_mode == kSettings) return 7;
    if (g_ui_mode == kMusic) return (int)g_entries.size() + 5;
    return 0;
}

void set_paused(int paused) {
    if (g_host && g_host->set_paused) g_host->set_paused(paused);
}

void close_ui() {
    if (g_decode.valid()) g_decode_cancelled = true;
    g_ui_open = false;
    g_ui_mode = kClosed;
    g_status.clear();
    set_paused(0);
}

void open_ui(UiMode mode) {
    if (!g_ui_open) {
        g_ui_open = true;
        set_paused(1);
    }
    g_ui_mode = mode;
    g_selection = 0;
    g_scroll = 0;
    g_status.clear();
    if (mode == kSettings) { g_settings = load_settings(); refresh_controllers(); }
    if (mode == kMusic) {
        g_entries = g_settings.music;
        g_music_selection = g_entries.empty() ? -1 : 0;
    }
}

void commit_settings() {
    save_settings(g_settings);
    if (apply_video(g_settings)) g_want_relaunch.store(true);
    if (g_host && g_host->reload_bindings) g_host->reload_bindings();
    close_ui();
}

void commit_music() {
    if (g_decode.valid()) return;
    g_settings.music = g_entries;
    save_settings(g_settings);
    // Editing a playlist in F1 must never replace the game disc early.
    if (!music_cd_requested()) { close_ui(); return; }
    if (g_entries.empty()) { g_status = "ADICIONE AO MENOS UM MP3"; return; }
    begin_music_decode();
}

void poll_music() {
    // Returning to the main mode menu (state 1) restores the original CD.
    if (!g_music_sectors.empty() && (!game_module_loaded() ||
        (psx_read_word(0x800763C4u) == 1 && psx_read_word(0x800763CCu) == 1))) {
        vib_set_cdda_playlist(nullptr, nullptr, 0);
        g_music_sectors.clear();
    }
    const bool requested = music_cd_requested();
    if (!requested) g_cd_request_seen = false;
    if (requested && !g_cd_request_seen && !g_ui_open) {
        g_cd_request_seen = true;
        g_settings = load_settings();
        open_ui(kMusic);
        if (!g_settings.music.empty()) begin_music_decode();
    }
    if (!g_decode.valid() || g_decode.wait_for(std::chrono::seconds(0)) != std::future_status::ready) return;
    PreparedMusic result = g_decode.get();
    if (g_decode_cancelled || !requested || !g_ui_open) return; // cancelled or left the request
    if (!result.error.empty() || result.tracks.empty()) {
        g_status = result.error.empty() ? "ADICIONE AO MENOS UM MP3" : result.error;
        return;
    }
    std::vector<const uint8_t*> pointers;
    std::vector<uint32_t> counts;
    for (const auto& track : result.tracks) {
        pointers.push_back(track.data());
        counts.push_back((uint32_t)(track.size() / kSectorBytes));
    }
    if (!vib_set_cdda_playlist(pointers.data(), counts.data(), (uint32_t)counts.size())) {
        g_status = "FALHA AO MONTAR O CD";
        return;
    }
    g_music_sectors = std::move(result.tracks);
    psx_disc_tray_cycle(0, PSX_DISC_TRAY_ORIGIN_MENU);
    close_ui();
}

void move_selection(int delta) {
    const int count = row_count();
    if (count <= 0) return;
    g_selection = std::max(0, std::min(count - 1, g_selection + delta));
    if (g_ui_mode == kMusic && g_selection < (int)g_entries.size()) g_music_selection = g_selection;
    if (g_selection < g_scroll) g_scroll = g_selection;
    if (g_selection >= g_scroll + kRows) g_scroll = g_selection - kRows + 1;
}

void cycle_settings(int delta) {
    if (g_ui_mode != kSettings) return;
    if (g_selection == 0) {
        int index = 0;
        for (size_t i = 0; i < 7; ++i)
            if (g_settings.resolution == kResValues[i]) index = (int)i;
        index = (index + delta + 7) % 7;
        g_settings.resolution = kResValues[index];
    } else if (g_selection == 1) {
        g_settings.fullscreen = (g_settings.fullscreen + delta + 3) % 3;
    } else if (g_selection == 2) {
        int index = 0;
        for (size_t i = 0; i < g_controllers.size(); ++i)
            if (g_settings.controller == g_controllers[i].guid) index = (int)i + 1;
        const int count = (int)g_controllers.size() + 1;
        index = (index + delta + count) % count;
        g_settings.controller = index == 0 ? "auto" : g_controllers[index - 1].guid;
    } else if (g_selection == 3) {
        g_settings.offset_ms = std::max(-2000, std::min(2000, g_settings.offset_ms + delta * 25));
    }
}

void activate() {
    if (g_ui_mode == kSettings) {
        if (g_selection == 4) {
            open_ui(kMusic);
        } else if (g_selection == 5) {
            commit_settings();
        } else if (g_selection == 6) {
            close_ui();
        }
        return;
    }
    if (g_ui_mode == kMusic) {
        const int base = (int)g_entries.size();
        if (g_selection == base) {
            g_want_pick_music = true;
        } else if (g_selection == base + 1) {
            if (g_music_selection >= 0 && (size_t)g_music_selection < g_entries.size()) {
                g_entries.erase(g_entries.begin() + g_music_selection);
                g_music_selection = g_entries.empty() ? -1 : std::min(g_music_selection, (int)g_entries.size() - 1);
                move_selection(-1);
                g_status = "MP3 REMOVIDO";
            }
        } else if (g_selection == base + 2) {
            g_entries.clear();
            g_music_selection = -1;
            g_scroll = 0;
            g_selection = 0;
            g_status = "LISTA LIMPA";
        } else if (g_selection == base + 3) {
            commit_music();
        } else if (g_selection == base + 4) {
            close_ui();
        }
        return;
    }
}

void go_back() {
    close_ui();
}

void adjust(int delta) {
    if (g_ui_mode == kSettings) cycle_settings(delta);
    else move_selection(delta);
}

bool axis_nav_ready() {
    const auto now = std::chrono::steady_clock::now();
    if (now - g_last_axis_nav < std::chrono::milliseconds(180)) return false;
    g_last_axis_nav = now;
    return true;
}

void handle_event(const SDL_Event* e) {
    if (g_decode.valid()) {
        if (e->type == SDL_EVENT_KEY_DOWN && e->key.scancode == SDL_SCANCODE_ESCAPE) close_ui();
        if (e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN && e->gbutton.button == SDL_GAMEPAD_BUTTON_EAST) close_ui();
        return;
    }
    if (e->type == SDL_EVENT_KEY_DOWN) {
        if (e->key.repeat) return;
        switch (e->key.scancode) {
        case SDL_SCANCODE_F1: close_ui(); return;
        case SDL_SCANCODE_ESCAPE: go_back(); return;
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W: move_selection(-1); return;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S: move_selection(1); return;
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A: adjust(-1); return;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D: adjust(1); return;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_SPACE: activate(); return;
        case SDL_SCANCODE_BACKSPACE: adjust(-1); return;
        default: return;
        }
    }
    if (e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        switch (e->gbutton.button) {
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: move_selection(-1); return;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: move_selection(1); return;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: adjust(-1); return;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: adjust(1); return;
        case SDL_GAMEPAD_BUTTON_SOUTH: activate(); return;
        case SDL_GAMEPAD_BUTTON_EAST:
        case SDL_GAMEPAD_BUTTON_BACK: go_back(); return;
        case SDL_GAMEPAD_BUTTON_GUIDE:
        case SDL_GAMEPAD_BUTTON_START:
        case SDL_GAMEPAD_BUTTON_NORTH: close_ui(); return;
        default: return;
        }
    }
    if (e->type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
        const int value = e->gaxis.value;
        if (std::abs(value) < 16000 ||
            (e->gaxis.axis != SDL_GAMEPAD_AXIS_LEFTX && e->gaxis.axis != SDL_GAMEPAD_AXIS_LEFTY) ||
            !axis_nav_ready()) return;
        if (e->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTX) adjust(value > 0 ? 1 : -1);
        else if (e->gaxis.axis == SDL_GAMEPAD_AXIS_LEFTY) move_selection(value > 0 ? 1 : -1);
    }
}

void rasterize(std::vector<uint32_t>& px) {
    px.assign(kUiW * kUiH, 0xFF080B11u);
    fill_rect(px.data(), 0, 0, kUiW, 50, 0xFF141A26u);
    fill_rect(px.data(), 0, 50, kUiW, 2, 0xFFFFD24Du);
    draw_text(px.data(), 24, 15, "VIB-RIBBON - CONFIGURACOES", 0xFFFFD24Du, 2);

    if (g_ui_mode == kSettings) {
        draw_text(px.data(), 24, 60, "F1 FECHA - SETAS MOVEM - ESQUERDA/DIREITA ALTERAM", 0xFFAEB6C4u, 1);
        char offset[32];
        std::snprintf(offset, sizeof(offset), "%+d MS", g_settings.offset_ms);
        draw_row(px.data(), g_selection, 0,
                 std::string("RESOLUCAO INTERNA : ") + resolution_label(g_settings));
        draw_row(px.data(), g_selection, 1,
                 std::string("TELA INTEIRA      : ") + kFullLabels[std::max(0, std::min(2, g_settings.fullscreen))]);
        draw_row(px.data(), g_selection, 2,
                 std::string("CONTROLE USB      : ") + controller_label(g_settings));
        draw_row(px.data(), g_selection, 3,
                 std::string("OFFSET DE AUDIO   : ") + offset);
        draw_row(px.data(), g_selection, 4, "MUSICA DO CD      : EDITAR MP3");
        draw_row(px.data(), g_selection, 5, "SALVAR E FECHAR");
        draw_row(px.data(), g_selection, 6, "CANCELAR");
    } else if (g_ui_mode == kMusic) {
        draw_text(px.data(), 24, 60, "PLAYLIST MP3 - ESCOLHA UMA FAIXA PARA REMOVER", 0xFFAEB6C4u, 1);
        const int base = (int)g_entries.size();
        for (int row = 0; row < kRows; ++row) {
            const int index = g_scroll + row;
            if (index < base) {
                char prefix[16];
                std::snprintf(prefix, sizeof(prefix), "%02d ", index + 1);
                draw_row(px.data(), g_selection - g_scroll, row, std::string(prefix) + file_label(g_entries[index]));
            } else if (index == base) {
                draw_row(px.data(), g_selection - g_scroll, row, "ADICIONAR MP3...");
            } else if (index == base + 1) {
                draw_row(px.data(), g_selection - g_scroll, row, "REMOVER MP3 SELECIONADO");
            } else if (index == base + 2) {
                draw_row(px.data(), g_selection - g_scroll, row, "LIMPAR LISTA");
            } else if (index == base + 3) {
                draw_row(px.data(), g_selection - g_scroll, row, "USAR ESTES MP3 (CONFIRMAR)");
            } else if (index == base + 4) {
                draw_row(px.data(), g_selection - g_scroll, row, "VOLTAR (CANCELAR)");
            }
        }
    }

    fill_rect(px.data(), 0, 456, kUiW, 24, 0xFF141A26u);
    if (!g_status.empty()) draw_text(px.data(), 24, 462, fit(g_status, 74), 0xFFFF9B9Bu, 1);
    else draw_text(px.data(), 24, 462, "A CONFIRMA - B VOLTA - F1 FECHA", 0xFF8E96A5u, 1);
}

void pick_music_files() {
    static const wchar_t kFilter[] =
        L"Arquivos MP3 (*.mp3)\0*.mp3\0Todos os arquivos (*.*)\0*.*\0\0";
    std::vector<wchar_t> buffer(32768, L'\0');
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetForegroundWindow();
    ofn.lpstrFilter = kFilter;
    ofn.lpstrFile = buffer.data();
    ofn.nMaxFile = (DWORD)buffer.size();
    ofn.lpstrInitialDir = g_pick_dir.c_str();
    ofn.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST |
                OFN_PATHMUSTEXIST | OFN_LONGNAMES | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) return;

    std::vector<std::wstring> chosen;
    const wchar_t* p = buffer.data();
    if (!*p) return;
    const std::wstring first = p;
    p += first.size() + 1;
    if (*p) {
        std::wstring dir = first;
        if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
        for (; *p; p += std::wcslen(p) + 1) chosen.push_back(dir + p);
    } else {
        chosen.push_back(first);
    }
    if (chosen.empty()) return;

    std::lock_guard<std::mutex> lock(g_ui_mutex);
    int added = 0;
    for (const auto& path : chosen) {
        if (std::find(g_entries.begin(), g_entries.end(), path) != g_entries.end()) continue;
        g_entries.push_back(path);
        ++added;
    }
    if (added) {
        g_pick_dir = std::filesystem::path(chosen.front()).parent_path().wstring();
        if (g_pick_dir.empty()) g_pick_dir = g_exe_dir;
        g_selection = (int)g_entries.size() - 1;
        g_music_selection = g_selection;
        g_scroll = std::max(0, g_selection - kRows + 1);
    }
    g_status = added ? "MP3 ADICIONADO - CONFIRME COM USAR ESTES MP3"
                     : "ESTES MP3 JA ESTAO NA LISTA";
}

}

extern "C" int vib_overlay_image(const uint32_t** pixels, int* w, int* h) {
    static std::vector<uint32_t> panel(kUiW * kUiH, 0xFF080B11u);
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    if (!g_ui_open) return 0;
    rasterize(panel);
    *pixels = panel.data();
    *w = kUiW;
    *h = kUiH;
    return 1;
}

extern "C" void vib_note_cdda_play(void) {
}

namespace {

int attach(void*, void*, void*, const PsxMenuHost* host) {
    g_host = host;
    set_exe_dir();
    refresh_controllers();
    const Settings settings = load_settings();
    g_settings = settings;
    if (!std::getenv("VIB_SETTINGS_NO_VIDEO")) apply_video(settings);
    psx_host_menu_set_image_provider(vib_overlay_image);
    std::fprintf(stdout, "vib-settings: menu desenhado dentro do jogo; F1 abre configuracoes\n");
    std::fflush(stdout);
    return 1;
}

int event(void*, const void* sdl_event) {
    if (!sdl_event) return 0;
    const SDL_Event* e = static_cast<const SDL_Event*>(sdl_event);
    // Keep window-close and device hotplug visible to the runtime even with the menu open.
    if (e->type == SDL_EVENT_QUIT || e->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) return 0;
    if (e->type == SDL_EVENT_GAMEPAD_ADDED || e->type == SDL_EVENT_GAMEPAD_REMOVED ||
        e->type == SDL_EVENT_GAMEPAD_REMAPPED) {
        std::lock_guard<std::mutex> lock(g_ui_mutex);
        refresh_controllers();
        return 0;
    }
    bool pick = false;
    {
        std::lock_guard<std::mutex> lock(g_ui_mutex);
        if (!g_ui_open) {
            if (e->type == SDL_EVENT_KEY_DOWN && !e->key.repeat &&
                e->key.scancode == SDL_SCANCODE_F1) {
                open_ui(kSettings);
                return 1;
            }
            if (e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN &&
                e->gbutton.button == SDL_GAMEPAD_BUTTON_GUIDE) {
                open_ui(kSettings);
                return 1;
            }
            return 0;
        }
        handle_event(e);
        pick = g_want_pick_music;
        g_want_pick_music = false;
    }
    if (pick) pick_music_files();
    return 1;
}

int is_open(void*) {
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    poll_music();
    return g_ui_open ? 1 : 0;
}

void set_open(void*, int open) {
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    if (open) open_ui(kSettings);
    else if (g_ui_open) close_ui();
}

void draw(void*, int, int) {
    if (g_want_relaunch.exchange(false) && g_host && g_host->request_exit)
        g_host->request_exit(PSX_MENU_EXIT_RELAUNCH);
}

void detach(void*) {
    psx_host_menu_set_image_provider(nullptr);
    std::lock_guard<std::mutex> lock(g_ui_mutex);
    g_ui_open = false;
    g_ui_mode = kClosed;
    g_host = nullptr;
}

const PsxMenuProvider kProvider = {
    PSX_MENU_ABI_VERSION,
    "Vib-Ribbon Settings",
    nullptr,
    attach,
    event,
    is_open,
    set_open,
    draw,
    detach
};

#ifndef VIB_SETTINGS_TEST
struct Register {
    Register() {
        const char* disable = std::getenv("VIB_SETTINGS_DISABLE");
        if (!disable || std::strcmp(disable, "1") != 0)
            psx_menu_set_provider(&kProvider);
    }
} g_register;
#endif

}
