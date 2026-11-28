#define VIB_SETTINGS_TEST
#include "../src/vib_settings.cpp"
#include <cassert>
#include <iostream>
static std::vector<uint8_t> test_ram(2 * 1024 * 1024);
extern "C" uint32_t psx_read_word(uint32_t addr) {
    uint32_t v; std::memcpy(&v, test_ram.data() + (addr & 0x1fffff), 4); return v;
}
int main() {
    for (const auto& name : {L".local/tone-22050-1.mp3", L".local/tone-48000-2.mp3"}) {
        std::vector<uint8_t> sectors, shifted;
        std::string error;
        assert(decode_mp3(name, 0, sectors, error));
        std::cout << "decoded sectors: " << sectors.size()/kSectorBytes << std::endl;
        assert(sectors.size() >= 599 * kSectorBytes && sectors.size() <= 610 * kSectorBytes);
        uint64_t energy = 0;
        for (size_t i = 0; i < sectors.size(); i += 4) {
            int16_t l, r;
            std::memcpy(&l, sectors.data()+i, 2); std::memcpy(&r, sectors.data()+i+2, 2);
            assert(std::abs((int)l - r) < 16);
            energy += std::abs((int)l);
        }
        assert(energy > 1000000);
        assert(decode_mp3(name, 250, shifted, error));
        assert(shifted.size() < sectors.size());
        std::cout << "PASS: resampling and offset, " << sectors.size()/kSectorBytes << " CD sectors\n";
    }
    std::vector<uint8_t> invalid; std::string error;
    assert(!decode_mp3(L"game.toml", 0, invalid, error));
    for (const auto& item : {std::pair<const char*, bool>{".local/custom-request.ram", true},
                            {".local/custom-menu.ram", false}}) {
        std::ifstream f(item.first, std::ios::binary);
        assert(f.read((char*)test_ram.data(), test_ram.size()));
        assert(music_cd_requested() == item.second);
    }
    std::fill(test_ram.begin(), test_ram.end(), 0);
    assert(!music_cd_requested());
    g_ui_mode = kMusic; g_entries.assign(12, L"track.mp3");
    move_selection(11);
    assert(g_selection == 11 && g_scroll == 3 && g_music_selection == 11);
    std::vector<uint32_t> pixels; rasterize(pixels);
    assert(pixels[(76+8*kRowH)*kUiW+20] == 0xFFFFD24D);
    std::cout << "PASS: invalid MP3, CD request gate, playlist scrolling and highlight\n";
}
