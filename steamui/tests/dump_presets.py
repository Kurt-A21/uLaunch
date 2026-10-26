import sys; sys.path.insert(0, sys.argv[1])
from oc_profile import PRESETS
for name, kv in PRESETS.items():
    for k, v in kv.items(): print(name, k, v)
