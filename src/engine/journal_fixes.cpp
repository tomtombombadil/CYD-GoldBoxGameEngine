// The Steam releases' journal text, corrected (Tom, 2026-11-26: "That's
// important!"). Their Gold Box Companion's Game.dat was typed, its typists'
// slips in it ("My names is Jarbarkas", "beleive"); its apostrophes are
// often a space ("I ll", "didn t"); and the sections taken from the PDFs
// (Introductions, tavern tales) are the scans' OCR ("~i INTRODUCTION ~",
// "part}'1", "Uorkh" for Llorkh, a tale's ornament read as "0 ", " ." for
// "."). Each fix below was found by setting the made journal beside the
// PDF's text and a second OCR of its pages, and checked there by eye: what
// the printed journal says (its own spellings and slips kept - "villanous").
// A journal is known by its Game.dat (its size and first 4 KB): another
// copy's fixes never touch it.
#include "journal_fixes.h"

#include <string.h>

namespace journal {

namespace {

constexpr uint32_t kPoolRad = 0x73366c8b;
constexpr uint32_t kCurse = 0xcc7abe84;
constexpr uint32_t kSecret = 0xa0095957;
constexpr uint32_t kDarkness = 0xdbbf33dd;
constexpr uint32_t kChampions = 0x57472e23;
constexpr uint32_t kDeathKnights = 0x0a55fa96;
constexpr uint32_t kDarkQueen = 0xd2f6d48f;
constexpr uint32_t kGateway = 0xaacc44c2;
constexpr uint32_t kTreasures = 0xc17e5f3b;

struct Fix {
    uint32_t    journal;
    char        kind;
    uint16_t    number;
    const char* from;       // every place in the entry
    const char* to;
};

const Fix kFixes[] = {
    // POOLRAD
    {kPoolRad, 'J', 2, "I never like pain", "I never liked pain"},
    {kPoolRad, 'J', 3, "unnatural forces must", "unnatural force must"},
    {kPoolRad, 'J', 5, "family treasurers", "family treasures"},
    {kPoolRad, 'J', 6, "gain style passage", "gain safe passage"},
    {kPoolRad, 'J', 7, "plane to place along", "plane to plane along"},
    {kPoolRad, 'J', 7, "on this plant.", "on this plane."},
    {kPoolRad, 'J', 11, "He though on", "He thought on"},
    {kPoolRad, 'J', 19, "in the half of", "in the hall of"},
    {kPoolRad, 'J', 20, "I fit out", "I lit out"},
    {kPoolRad, 'J', 20, "Not bein a", "Not bein' a"},
    {kPoolRad, 'J', 21, "Bur the flame", "But the flame"},
    {kPoolRad, 'J', 25, "Under tour", "Under your"},
    {kPoolRad, 'J', 26, "Yarash mush", "Yarash must"},
    {kPoolRad, 'J', 27, "jBring", "Bring"},
    {kPoolRad, 'J', 38, "secretly hats", "secretly hates"},
    {kPoolRad, 'J', 43, "The vampires shouted", "The vampire shouted"},
    {kPoolRad, 'J', 44, "glitter n the", "glitter in the"},
    {kPoolRad, 'J', 45, "trinkets was we", "trinkets as we"},
    {kPoolRad, 'J', 45, "and se were", "and so were"},
    {kPoolRad, 'J', 46, "Ding", "Dirg"},
    {kPoolRad, 'J', 46, "while wondering", "while wandering"},
    {kPoolRad, 'J', 47, "Heart ettins", "Heard ettins"},
    {kPoolRad, 'J', 51, "trouble then the", "trouble than the"},
    {kPoolRad, 'J', 52, "with you fire", "with your fire"},
    // CURSE
    {kCurse, 'J', 18, "cresent", "crescent"},
    {kCurse, 'J', 29, "flame, skim from", "flame, skip from"},
    {kCurse, 'J', 30, "Impercentor", "Imperceptor"},
    {kCurse, 'J', 36, "the easy party.", "the easy part."},
    {kCurse, 'J', 36, "off the Teshwave", "off to the Teshwave"},
    {kCurse, 'J', 36, "Good look.", "Good luck."},
    {kCurse, 'J', 46, "altering", "alerting"},
    // SECRET
    {kSecret, 'J', 10, "the alter.", "the altar."},
    {kSecret, 'J', 12, "Folks was rules", "Folks was ruled"},
    {kSecret, 'J', 14, "hand with never", "hand will never"},
    {kSecret, 'J', 15, "THE FRONT GIANT", "THE FROST GIANT"},
    {kSecret, 'J', 16, "allies will fail;", "allies will fall;"},
    {kSecret, 'J', 23, "The mages keeps", "The mages keep"},
    {kSecret, 'J', 33, "by that made spirit", "by that mad spirit"},
    {kSecret, 'J', 34, "skulking to ruins", "skulking the ruins"},
    {kSecret, 'J', 43, "for use,", "for us,"},
    {kSecret, 'J', 43, "has to cloth,", "has to clothe,"},
    {kSecret, 'J', 44, "The scam started", "The scum started"},
    {kSecret, 'J', 55, "I had contracts", "I had contacts"},
    {kSecret, 'J', 61, "Dreadlord, you usefulness", "Dreadlord, your usefulness"},
    {kSecret, 'J', 61, "Hillsfar any my", "Hillsfar and my"},
    {kSecret, 'J', 63, "what you now", "what you know"},
    // Pools of Darkness
    {kDarkness, 'J', 1, "and you to not", "and you'll not"},
    {kDarkness, 'J', 2, "or you to", "or you'll"},
    {kDarkness, 'J', 4, "alter", "altar"},
    {kDarkness, 'J', 33, "alter", "altar"},
    {kDarkness, 'J', 37, "ALTER", "ALTAR"},
    {kDarkness, 'J', 37, "alter", "altar"},
    {kDarkness, 'J', 55, "alter", "altar"},
    {kDarkness, 'J', 81, "ALTER", "ALTAR"},
    {kDarkness, 'J', 85, "ALTER", "ALTAR"},
    {kDarkness, 'J', 10, "the grave image", "the graven image"},
    {kDarkness, 'J', 16, "Council of Elvis!", "Council of Elves!"},
    {kDarkness, 'J', 46, "veeping", "weeping"},
    {kDarkness, 'J', 68, "but felt the need", "but fell the need"},
    {kDarkness, 'J', 74, "after this death", "after his death"},
    {kDarkness, 'J', 78, "brethern", "brethren"},
    {kDarkness, 'J', 84, "one of who", "one of whom"},
    {kDarkness, 'J', 88, "holds up the Wand", "holds up the Ward"},
    {kDarkness, 'J', 90, "Plhan", "Phlan"},
    // Champions of Krynn
    {kChampions, 'J', 14, "Now, all you", "Now, aaall you"},
    {kChampions, 'J', 18, "elite units is", "elite units in"},
    {kChampions, 'J', 28, "nobody's even", "nobody's ever"},
    {kChampions, 'J', 73, "on the alter!", "on the altar!"},
    {kChampions, 'J', 80, "Drangonlances", "Dragonlances"},
    {kChampions, 'J', 87, "they night", "they might"},
    {kChampions, 'T', 16, "clawto-claw", "claw-to-claw"},
    // Death Knights of Krynn
    {kDeathKnights, 'J', 7, "doesn't t seem", "doesn't seem"},
    {kDeathKnights, 'J', 11, "There so much", "There's so much"},
    {kDeathKnights, 'J', 20, "need to unit against his common", "need to unite against this common"},
    {kDeathKnights, 'J', 20, "villages before his,", "villages before this,"},
    {kDeathKnights, 'J', 20, "countless light", "countless tight"},
    {kDeathKnights, 'J', 22, "Cereberus", "Cerberus"},
    {kDeathKnights, 'J', 23, "You beleive me to to be", "You believe me to be"},
    {kDeathKnights, 'J', 23, "groteque", "grotesque"},
    {kDeathKnights, 'J', 25, "The rest of your --", "The rest of you --"},
    {kDeathKnights, 'J', 28, "VAGABOUND", "VAGABOND"},
    {kDeathKnights, 'J', 29, "We meus save", "We must save"},
    {kDeathKnights, 'J', 30, "conventionally.", "conversationally."},
    {kDeathKnights, 'J', 33, "having mader a", "having made a"},
    {kDeathKnights, 'J', 33, "also notice", "also noticed"},
    {kDeathKnights, 'J', 33, "they think it", "they think is"},
    {kDeathKnights, 'J', 34, "rises an", "rises and"},
    {kDeathKnights, 'J', 36, "grapped", "grabbed"},
    {kDeathKnights, 'J', 39, "A greatly rumbling", "A gently rumbling"},
    {kDeathKnights, 'J', 41, "he sounts,", "he spouts,"},
    {kDeathKnights, 'J', 42, "villagnous", "villanous"},
    {kDeathKnights, 'J', 42, "situtation", "situation"},
    {kDeathKnights, 'J', 42, "trival", "trivial"},
    {kDeathKnights, 'J', 42, "reveals about the most", "reveals about that most"},
    {kDeathKnights, 'J', 43, "bazzar.", "bazaar."},
    {kDeathKnights, 'J', 43, "Cimb", "Climb"},
    {kDeathKnights, 'J', 43, "this fate", "this fake"},
    {kDeathKnights, 'J', 45, "boldily", "boldly"},
    {kDeathKnights, 'J', 46, "Thanks Paladine", "Thank Paladine"},
    {kDeathKnights, 'J', 46, "told Diane", "told Daine"},
    {kDeathKnights, 'J', 48, "Never before his", "Never before has"},
    {kDeathKnights, 'J', 48, "naimed", "nailed"},
    {kDeathKnights, 'J', 48, "door to me armory", "door to the armory"},
    {kDeathKnights, 'J', 48, "these accused", "these accursed"},
    {kDeathKnights, 'J', 50, "lake. You equipment", "lake. Your equipment"},
    {kDeathKnights, 'J', 50, "quckly", "quickly"},
    {kDeathKnights, 'J', 52, "PHILOSPHER'S", "PHILOSOPHER'S"},
    {kDeathKnights, 'J', 53, "king fold", "king folds"},
    {kDeathKnights, 'J', 53, "Roots spout", "Roots sprout"},
    {kDeathKnights, 'J', 54, "multilated", "mutilated"},
    {kDeathKnights, 'J', 55, "deadily.", "deadly."},
    {kDeathKnights, 'J', 56, "undead board", "undead boar"},
    {kDeathKnights, 'J', 57, "from ths place", "from this place"},
    {kDeathKnights, 'J', 58, "Brave adventures.", "Brave adventurers,"},
    {kDeathKnights, 'J', 58, "from coing", "from coming"},
    {kDeathKnights, 'J', 58, "the 165th", "the 16th"},
    {kDeathKnights, 'J', 59, "Sincer we", "Since we"},
    {kDeathKnights, 'J', 60, "Our kind", "Our king"},
    {kDeathKnights, 'J', 61, "body is a", "body is as"},
    {kDeathKnights, 'J', 62, "Durfey changes", "Durfey charges"},
    {kDeathKnights, 'J', 63, "door in e floor", "door in the floor"},
    {kDeathKnights, 'J', 64, "a blond", "a blonde"},
    {kDeathKnights, 'J', 65, "ANNUAL ACTION!", "ANNUAL AUCTION!"},
    {kDeathKnights, 'J', 65, "skull carriers", "skull carries"},
    {kDeathKnights, 'J', 65, "enchangement", "enchantment"},
    // The Dark Queen of Krynn
    {kDarkQueen, 'I', 1, ",* INTRODUCTION tw", "INTRODUCTION"},
    {kDarkQueen, 'I', 1, "QENERAL", "GENERAL"},
    {kDarkQueen, 'I', 1, "\\Nord", "Word"},
    {kDarkQueen, 'J', 3, "stand guard at her", "stand guard at the"},
    {kDarkQueen, 'J', 3, "break this", "break the"},
    {kDarkQueen, 'J', 7, "and they will", "and they wish to"},
    {kDarkQueen, 'J', 7, "Catacalysm", "Cataclysm"},
    {kDarkQueen, 'J', 11, "vengeance lurking", "vengeance lurks"},
    {kDarkQueen, 'J', 16, "a bit of ruins", "a bit of ruin"},
    {kDarkQueen, 'J', 22, "controlling those", "controlling these"},
    {kDarkQueen, 'J', 24, "organized, and the", "organized, and they"},
    {kDarkQueen, 'J', 24, "the Sharkman", "the Sharkmen"},
    {kDarkQueen, 'J', 32, "Some of these", "Some of the"},
    {kDarkQueen, 'J', 32, "enormous spider", "enormous spiders"},
    {kDarkQueen, 'J', 36, "you are to sign", "you are to assign"},
    {kDarkQueen, 'J', 40, "cacaphonous", "cacophonous"},
    {kDarkQueen, 'J', 44, "Hiddukul.", "Hiddukel."},
    {kDarkQueen, 'J', 45, "boat fro", "boat for"},
    {kDarkQueen, 'J', 47, "able to compete", "able to complete"},
    {kDarkQueen, 'J', 56, "fogue", "forge"},
    {kDarkQueen, 'J', 56, "Aldinachru", "Aldinanachru"},
    {kDarkQueen, 'J', 59, "incarcerated", "incinerated"},
    {kDarkQueen, 'J', 61, "the onles", "the ones"},
    {kDarkQueen, 'J', 65, "fuming pots", "fuming pools"},
    {kDarkQueen, 'J', 44, "Glorius Thenol", "Glorious Thenol"},
    // Gateway to the Savage Frontier
    {kGateway, 'J', 2, "faces. The came", "faces. They came"},
    {kGateway, 'J', 21, "hire adventures", "hire adventurers"},
    {kGateway, 'J', 23, "One bye one the undead the", "One by one the undead they"},
    {kGateway, 'J', 24, "by and angry looking", "by an angry-looking"},
    {kGateway, 'J', 25, "do i,", "do it,"},
    {kGateway, 'J', 25, "Once cleric", "One cleric"},
    {kGateway, 'J', 27, "Alarun", "Alaraun"},
    {kGateway, 'J', 27, "full-scale ware!", "full-scale war!"},
    {kGateway, 'J', 29, "Zhentarium", "Zhentarim"},
    {kGateway, 'J', 31, "Zhentarium", "Zhentarim"},
    {kGateway, 'J', 33, "Zhentarium", "Zhentarim"},
    {kGateway, 'J', 29, "Musn't", "Mustn't"},
    {kGateway, 'J', 30, "Nashar's", "Nasher's"},
    {kGateway, 'J', 31, "who cal", "who can"},
    {kGateway, 'J', 59, "hear tour", "hear your"},
    {kGateway, 'J', 59, "Sundbar", "Sundabar"},
    {kGateway, 'J', 62, "High Forrest", "High Forest"},
    // Treasures of the Savage Frontier
    {kTreasures, 'I', 1, "~i INTRODUCTION ~", "INTRODUCTION"},
    {kTreasures, 'I', 1, "part}'1", "party,"},
    {kTreasures, 'I', 1, "Uorkh", "Llorkh"},
    {kTreasures, 'I', 1, "surviving ores", "surviving orcs"},
    {kTreasures, 'I', 1, "Cjreat", "Great"},
    {kTreasures, 'I', 1, "Cjeildarr", "Geildarr"},
    {kTreasures, 'I', 1, "every tum,", "every turn,"},
    {kTreasures, 'J', 10, "My names is", "My name is"},
    {kTreasures, 'J', 10, "more than more.", "more than one."},
    {kTreasures, 'J', 12, "Our vessels slowly", "Our vessel slowly"},
    {kTreasures, 'J', 13, "EXPLANATIONS", "EXPLANATION"},
    {kTreasures, 'J', 16, "what information if", "what information it"},
    {kTreasures, 'J', 33, "told bus,", "told us,"},
    {kTreasures, 'J', 39, "The farmer around", "The farmers around"},
    {kTreasures, 'J', 48, "they say of the", "they say on the"},
    {kTreasures, 'J', 53, "for long is this", "for long in this"},
    {kTreasures, 'J', 54, "for you heroism", "for your heroism"},
    {kTreasures, 'J', 55, "coastal cites.", "coastal cities."},
    {kTreasures, 'J', 66, "Great adventures!", "Great adventurers!"},
    {kTreasures, 'J', 71, "had no hands in", "had no hand in"},
    {kTreasures, 'J', 74, "THE NEW OF", "THE NEWS OF"},
};

bool lower(char c) { return c >= 'a' && c <= 'z'; }
bool letter(char c) { return lower(c) || (c >= 'A' && c <= 'Z'); }

// Replaces text[at, at + n) with s (false: no room)
bool splice(char* text, size_t* len, size_t cap, size_t at, size_t n, const char* s)
{
    const size_t m = strlen(s);
    if (*len - n + m > cap) return false;
    memmove(text + at + m, text + at + n, *len - at - n);
    memcpy(text + at, s, m);
    *len = *len - n + m;
    return true;
}

} // namespace

uint32_t fix_key(dax::ByteSource& dat)
{
    if (dat.size() == 0) return 0;
    uint8_t b[256];
    uint32_t h = 2166136261u;
    for (uint32_t at = 0; at < 4096 && at < dat.size(); at += sizeof b) {
        const size_t want = dat.size() - at < sizeof b ? dat.size() - at : sizeof b;
        const size_t got = dat.read_at(at, b, want);
        for (size_t i = 0; i < got; ++i) h = (h ^ b[i]) * 16777619u;
        if (got < want) break;
    }
    return h;
}

size_t fix_text(uint32_t key, char kind, int number, char* text, size_t len, size_t cap)
{
    if (key == 0) return len;
    bool known = false;
    // This entry's own fixes, each everywhere in it
    for (const Fix& f : kFixes) {
        if (f.journal != key) continue;
        known = true;
        if (f.kind != kind || f.number != number) continue;
        const size_t n = strlen(f.from), m = strlen(f.to);
        for (size_t i = 0; n > 0 && i + n <= len;) {
            if (memcmp(text + i, f.from, n) != 0) {
                ++i;
                continue;
            }
            if (!splice(text, &len, cap, i, n, f.to)) break;
            i += m;
        }
    }
    if (!known) return len;            // (not a journal these were checked on)
    // A tale's ornament read as a letter before its heading ("0 Tavern Tale 1", "U Tavern Tale 12")
    if (len > 14 && text[0] != '"' && text[1] == ' ' && memcmp(text + 2, "Tavern Tale", 11) == 0)
        splice(text, &len, cap, 0, 2, "");
    for (size_t i = 1; i + 1 < len; ++i) {
        // An apostrophe typed as a space ("I ll", "didn t", "intruder s") - not in Gateway's ciphered notes
        if (key != kGateway && text[i] == ' ' && letter(text[i - 1])) {
            size_t k = i + 1;
            while (k < len && lower(text[k])) ++k;
            const size_t w = k - (i + 1);
            const bool end = k >= len || !(letter(text[k]) || text[k] == '\'' || text[k] == '-');
            if (end && w >= 1 && w <= 2) {
                const char* s = text + i + 1;
                if ((w == 1 && (*s == 's' || *s == 't' || *s == 'd' || *s == 'm')) ||
                    (w == 2 && (!memcmp(s, "ll", 2) || !memcmp(s, "ve", 2) || !memcmp(s, "re", 2))))
                    text[i] = '\'';
            }
        }
        // A space before a stop ("from you ." - the OCR's)
        if (text[i] == ' ' && (letter(text[i - 1]) || text[i - 1] == '"' || text[i - 1] == '\'') &&
            strchr(".,;:!?", text[i + 1]) && (i + 2 >= len || text[i + 2] == ' ' || text[i + 2] == '"' ||
                                              text[i + 2] == '\n' || text[i + 2] == '.' || text[i + 2] == '\''))
            splice(text, &len, cap, i, 1, "");
    }
    return len;
}

} // namespace journal
