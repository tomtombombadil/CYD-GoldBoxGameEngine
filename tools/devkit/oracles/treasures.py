#!/usr/bin/env python3
# oracle_treasures*.txt: Darkness's oracle generator on Treasures' program
# (treasures_port_facts.md P13: the same rules and tables at Treasures' places)
import os, sys
sys.argv[1:] = ['treasures']
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
exec(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'darkness.py')).read())
