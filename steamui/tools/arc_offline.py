#!/usr/bin/env python3
"""Runs uLaunch's arc (result-code generator) without network access.

arc normally downloads Atmosphere's *_results.hpp files from GitHub (an API call plus one download per file)
on every build, which fails when GitHub throttles the build machine, and it reads Atmosphere *master*, not the
Atmosphere-libs version uLaunch is built against. This wrapper feeds arc's Atmosphere mode the matching headers
from libs/Atmosphere-libs instead; everything else is arc unchanged.

  python3 steamui/tools/arc_offline.py gen_db default+./libs/uCommon/include/ul/ul_Results.rc.hpp
"""
import os, runpy, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ARC = os.path.join(ROOT, "arc")
AMS_RESULTS = os.path.join(ROOT, "libs", "Atmosphere-libs", "libvapours", "include", "vapours", "results")

sys.path.insert(0, ARC)
import arc_db_gen  # noqa: E402  (arc's own module; arc.py below imports this same instance)


def local_ams_rc_hpps():
    if not os.path.isdir(AMS_RESULTS):
        sys.exit(f"arc_offline: {AMS_RESULTS} is missing (check out submodules: git submodule update --init --recursive)")
    return sorted(os.path.join(AMS_RESULTS, f) for f in os.listdir(AMS_RESULTS) if f.endswith("_results.hpp"))


arc_db_gen.query_ams_rc_hpps = local_ams_rc_hpps
arc_db_gen.load_remote_rc_hpp = arc_db_gen.load_local_rc_hpp

sys.argv = [os.path.join(ARC, "arc.py")] + sys.argv[1:]
runpy.run_path(sys.argv[0], run_name="__main__")
