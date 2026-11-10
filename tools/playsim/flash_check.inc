// flash_check.inc - the monkey's check for messages too quick to read
// (Tom, 2026-11-10: "messages ... flash past so quickly you can't read
// them. That's not acceptable"). Mode `flashcheck` (sweep built with
// -DCYD_TEST_FLASH): after every tick the canvas's text rows - the text
// window under the 3D view (rows 17-22) and the menu line (24) - are
// compared with the tick before. A row whose words appear and are then
// replaced or wiped with no input between and under kFlashMs is reported
// "!! FLASH" with the screen, the words (the last put on that row, or the
// script's text) and how long they stayed. Writing that grows (the
// letters coming one by one) is not a change of words. Fights are left out
// (their pages keep the Speed setting, the player's own choice), so are
// typed fields (the cursor blinks). A tick is 20 ms: 40 ms short of kFlashMs is still a second. Env READTIME=0
// runs with Readable Messages off (the originals' timing) - it should find
// the flashes the comfort removes.
namespace flash {
constexpr uint32_t kFlashMs = 1000;
constexpr int kRow0 = 17, kRow1 = 24;
std::vector<uint8_t> prev[25];
uint32_t shown_at[25];
bool fresh[25];
char words[25][64];
char menu_then[25][96];
int menu_n_then[25];
uint32_t last_in = 0;
int found = 0, rows_seen = 0;
std::map<std::string, int> seen;

void input() { last_in = g_now; }

void row_px(int r, std::vector<uint8_t>& out)
{
    out.resize(320 * 8);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 320; ++x) out[y * 320 + x] = C.get(x, r * 8 + y);
}

void reset()
{
    for (int r = kRow0; r <= kRow1; ++r) {
        prev[r].clear();
        fresh[r] = false;
    }
}

void tick()
{
    using namespace play;
    if (!d || fg || input_mode != Input::None || screen == Screen::Title) {
        reset();
        return;
    }
    std::vector<uint8_t> now;
    for (int r = kRow0; r <= kRow1; ++r) {
        if (r == 23) continue;
        row_px(r, now);
        if (prev[r].empty()) {
            prev[r] = now;
            continue;
        }
        if (now == prev[r]) continue;
        // (the row's paper: its commonest colour - the Dark Queen's boxes are grey)
        int count[256] = {};
        for (uint8_t v : prev[r]) ++count[v];
        int bg = 0;
        for (int v = 1; v < 256; ++v)
            if (count[v] > count[bg]) bg = v;
        bool blank_prev = true, blank_now = true, grow = true;
        for (size_t i = 0; i < now.size(); ++i) {
            if (prev[r][i] != bg) blank_prev = false;
            if (now[i] != bg) blank_now = false;
            if (prev[r][i] != bg && now[i] != prev[r][i]) grow = false;
        }
        // (a menu the script replaced by its next one - not a message: both
        // times a menu up, no note)
        const bool menus = r == 24 && menu_n_then[r] > 0 && menu.count > 0 && !note_until;
        if (fresh[r] && !grow && !menus && last_in <= shown_at[r] && g_now - shown_at[r] + 40 < kFlashMs) {
            const std::string key = std::string(words[r]) + " @" + std::to_string(static_cast<int>(screen));
            if (seen[key]++ == 0) {
                ++found;
                printf("!! FLASH row %d screen %d: \"%s\" stayed %u ms (area %d block %d)\n", r, static_cast<int>(screen),
                       words[r], static_cast<unsigned>(g_now - shown_at[r]), d->gs.game_area, d->gs.script);
                if (const char* dir = getenv("FLASHSHOT")) {
                    // (the row before and after, as a PGM: 320 x 17, a line between)
                    char fn[256];
                    snprintf(fn, sizeof fn, "%s/flash_%d.pgm", dir, found);
                    if (FILE* f = fopen(fn, "wb")) {
                        fprintf(f, "P5 320 17 15\n");
                        fwrite(prev[r].data(), 1, prev[r].size(), f);
                        for (int x = 0; x < 320; ++x) fputc(8, f);
                        fwrite(now.data(), 1, now.size(), f);
                        fclose(f);
                    }
                }
                if (getenv("FLASHDBG")) printf("   then %s now [%s|%s]\n", menu_then[r], menu.prompt, menu.s);
                if (getenv("FLASHDBG")) printf("   note_until %d now %u held %d wait %d\n", (int)(note_until ? (int32_t)(note_until - g_now) : -99999), (unsigned)g_now, (int)note_held, vm ? (int)vm->wait() : -1);
            }
        }
        if (!blank_now && (!grow || blank_prev)) {
            fresh[r] = true;
            shown_at[r] = g_now;
            ++rows_seen;
            const char* w = g_put_text[r];
            if (vm && r < 23 && vm->wait() == ecl::Wait::Print && vm->text()[0]) w = vm->text();
            snprintf(words[r], sizeof words[r], "%s", w);
            menu_n_then[r] = note_until ? 0 : menu.count;
            snprintf(menu_then[r], sizeof menu_then[r], "[%s|%s] note %d", menu.prompt, menu.s, (int)(note_until ? (int32_t)(note_until - g_now) : -1));
        } else if (blank_now) {
            fresh[r] = false;
        }
        prev[r] = now;
    }
}

void report() { printf("flashcheck: %d message(s) shown too briefly (%d shown)\n", found, rows_seen); }
}  // namespace flash
