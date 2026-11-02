#!python

# Marks, on each layer map, the key that holds that layer.
#
# The layer keys are drawn through raw_binding_map, so keymap-drawer cannot
# tell which key activates which layer. The icon of each layer
# (layer_legend_map in the keymap-drawer config) is unique to it, so a base
# layer key carrying that icon is a key that holds the layer. The marked key
# gets its own class, layer-key: "held" is already worn by the modifiers.

import argparse
from pathlib import Path

import yaml


def legends(key):
    if isinstance(key, str):
        return {key}
    return {key.get("t"), key.get("h")}


def mark(key, icon):
    if isinstance(key, str):
        key = {"t": key} if key else {}
    # Keep what the key does on its own layer; the icon goes to the hold field
    if not key.get("t"):
        key["t"] = icon
    elif not key.get("h"):
        key["h"] = icon
    key["type"] = "layer-key"
    return key


parser = argparse.ArgumentParser()

parser.add_argument("config")
parser.add_argument("keymap")

args = parser.parse_args()

config = yaml.safe_load(Path(args.config).read_text())
icons = config["parse_config"]["layer_legend_map"]

# The style that enlarges the layer icons has to name each of them
style = config["draw_config"]["svg_extra_style"]
for icon in icons.values():
    name = icon.strip("$")
    for slot in ("tap", "hold"):
        if f'use.{slot}[class~="{name}"]' not in style:
            print(f'svg_extra_style has no use.{slot}[class~="{name}"] rule for a layer icon')
            raise SystemExit(1)

keymap_path = Path(args.keymap)
keymap = yaml.safe_load(keymap_path.read_text())
layers = keymap["layers"]
base = next(iter(layers.values()))

for layer, icon in icons.items():
    if layer not in layers:
        continue
    held = [position for position, key in enumerate(base) if icon in legends(key)]
    for position in held:
        layers[layer][position] = mark(layers[layer][position], icon)
    print(f"  {layer}: held key positions {held or 'none'}")

keymap_path.write_text(yaml.safe_dump(keymap, allow_unicode=True, sort_keys=False))
