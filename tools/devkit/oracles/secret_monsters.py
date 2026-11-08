#!/usr/bin/env python3
# oracle_secret_monsters.txt for behave secret's allmonsters part
# (behave_secret_monsters.inc). AD&D's Strength / Dexterity (gb.py);
# Secret's item numbering (profile.cpp: arrows 30, quarrels 12, the +1
# types of GAME.OVR 0x382C6: 22-26, 18, 19); the facts of its monsters'
# abilities (secret_facts_2.md 15, the original's code - as the .inc's
# comments record them): Alter's Level multipliers (level + 2), the golem's
# 3 rounds slowed, paralysis 10 rounds, the gaze's reach 6, the pyrohydra's
# 8, the umber hulk's 1d10 + 2 rounds, the hell hound's 7, the slug's 4d8.
import gb

w = gb.Writer('secret_monsters')
w.c('behave secret allmonsters oracle - tools/devkit/oracles/secret_monsters.py')
w.row('m_str_hit', [gb.STR_HIT[g] for g in range(31)])
w.row('m_str_dmg', [gb.STR_DMG[g] for g in range(31)])
w.row('m_max_enc', [gb.STR_ENC[g] for g in range(31)])
w.row('m_dex_ac', [gb.DEX_AC[d] for d in range(26)])
w.row('m_dex_react', [gb.DEX_MISSILE[d] for d in range(26)])
w.row('m_arrow', 30)
w.row('m_quarrel', 12)
w.row('m_hit_types', 22, 23, 24, 25, 26, 18, 19)
w.row('m_level_mul', 2, 3, 4, 5, 6)
w.row('m_golem_slow', 3)
w.row('m_para', 10)
w.row('m_reach', 6)
w.row('m_pyro', 8)
w.row('m_umber', 1, 10, 2)
w.row('m_hound', 7)
w.row('m_slug', 4, 8)
w.done()
