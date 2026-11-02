#!/usr/bin/env bash

CONFIG_ROOT="$(git rev-parse --show-toplevel)"

# merge_yaml.py needs PyYAML. The system python3 typically doesn't have it,
# but the keymap-drawer pipx venv does (PyYAML is one of its dependencies).
# Prefer the venv's interpreter by reading the shebang of the `keymap` binary;
# fall back to system python3/python if that fails.
KEYMAP_BIN="$(command -v keymap)"
if [[ -z "$KEYMAP_BIN" ]]; then
  echo "Error: 'keymap' not found in PATH. Install keymap-drawer first:" >&2
  echo "  pipx install keymap-drawer==0.23.0 && pipx ensurepath" >&2
  exit 1
fi

PYTHON="$(sed -n '1{s|^#!\([^[:space:]]*\).*|\1|;p;}' "$KEYMAP_BIN")"
if [[ ! -x "$PYTHON" ]]; then
  PYTHON="$(command -v python3 || command -v python)"
fi
if [[ -z "$PYTHON" ]] || [[ ! -x "$PYTHON" ]]; then
  echo "Error: no usable Python interpreter found" >&2
  exit 1
fi

# Quick sanity check that this Python can import yaml
if ! "$PYTHON" -c 'import yaml' >/dev/null 2>&1; then
  echo "Error: PyYAML is not available for $PYTHON." >&2
  echo "If you're using a system Python, install it with: pip install --user pyyaml" >&2
  exit 1
fi

# Locate keymap-drawer's cache directory in an OS-agnostic way.
# Falls back to common conventions if keymap-drawer can't tell us.
KD_CACHE="$("$PYTHON" -c 'from platformdirs import user_cache_dir; print(user_cache_dir("keymap-drawer"))' 2>/dev/null)"
if [[ -z "$KD_CACHE" ]]; then
  case "$(uname -s)" in
    Darwin) KD_CACHE="$HOME/Library/Caches/keymap-drawer" ;;
    *)      KD_CACHE="${XDG_CACHE_HOME:-$HOME/.cache}/keymap-drawer" ;;
  esac
fi

# A failed step must stop the script: an old map left in docs/ would pass for
# a fresh one. Set after the setup, whose lookups print errors or fall back.
set -eo pipefail

export_layer() {
  local KEYMAP_NAME=$1
  local LAYER_NAME=$2
  local LAYOUT_FILE=$3
  local EXTRA_LAYOUT="$4"
  local -a EXTRA_FLAG=()

  if [[ -n "$EXTRA_LAYOUT" ]]; then
    EXTRA_FLAG+=("-j" "$EXTRA_LAYOUT")
  fi

  echo "- Exporting layout map image for layer '$LAYER_NAME'"
  keymap -c "${CONFIG_ROOT}/support/keymap-config.yaml" draw \
    "${EXTRA_FLAG[@]}" \
    -s "$LAYER_NAME" \
    -o "${CONFIG_ROOT}/docs/images/$LAYOUT_FILE" \
    "$KEYMAP_NAME"
}

export_layout_map() {
  local SHIELD_NAME="$1"
  local KEYS_COUNT="$2"
  local EXTRA_LAYOUT="$3"
  local KEYMAP_NAME
  local KD_KEYMAP
  local KD_CONFIG_MAIN="${CONFIG_ROOT}/support/keymap-config.yaml"
  local KD_CONFIG_NO_SYMBOLS
  local KD_CONFIG_BACKGROUND
  local -a LAYOUT_LAYERS
  local -a LAYOUT_MAP_NAMES
  local -a EXTRA_FLAG=()

  KEYMAP_NAME="$(echo "${SHIELD_NAME}" | tr "[:upper:]" "[:lower:]")"
  KD_KEYMAP="$(mktemp).yaml"

  if [[ -n "$EXTRA_LAYOUT" ]]; then
    EXTRA_FLAG+=("-j" "$EXTRA_LAYOUT")
  fi

  echo "Generating layout map images for the ${SHIELD_NAME} keyboard..."
  echo "- Generating Keymap-Drawer YAML file"

  # The first part of the temp keymap contains the layout map with keys showing
  # their "Shift" symbol, and it include all base keymaps.
  keymap -c "$KD_CONFIG_MAIN" parse -z "${CONFIG_ROOT}/config/${KEYMAP_NAME}.keymap" \
    | sed -n '1,/Navigation:/p' \
    | sed -e '$ d' > "$KD_KEYMAP"

  # We need to create a special configuration for Keyboard Drawer that will be
  # the normal configuration, merged with the "no symbols" diff that we have on
  # the repository.
  KD_CONFIG_NO_SYMBOLS="$(mktemp).yaml"
  "$PYTHON" "${CONFIG_ROOT}/support/merge_yaml.py" \
    "$KD_CONFIG_MAIN" \
    "${CONFIG_ROOT}/support/keymap-config-no-shift-symbols.yaml" > "$KD_CONFIG_NO_SYMBOLS"

  # The second part of the file contains the rest of the layout map with keys NOT
  # showing their "Shift" symbol.
  keymap -c "$KD_CONFIG_NO_SYMBOLS" parse -z "${CONFIG_ROOT}/config/${KEYMAP_NAME}.keymap" \
    | sed -n '/Navigation:/,$ p' >> "$KD_KEYMAP"

  # On each layer map, draw the key that holds the layer in its own color and
  # with the layer icon.
  echo "- Marking the keys that hold each layer"
  "$PYTHON" "${CONFIG_ROOT}/support/mark_held_keys.py" "$KD_CONFIG_MAIN" "$KD_KEYMAP" || exit 1

  LAYOUT_LAYERS=(
    "QWERTY"
    "Navigation"
    "Numbers"
    "Symbols"
    "Media"
    "Mouse"
    "Functions"
    "Buttons"
    "System"
    "COLEMAK"
  )
  LAYOUT_MAP_NAMES=(
    "layer0-main"
    "layer1-navigation"
    "layer2-numbers"
    "layer3-symbols"
    "layer4-media"
    "layer5-mouse"
    "layer6-functions"
    "layer7-buttons"
    "layer8-system"
    "layer9-colemak"
  )

  for i in "${!LAYOUT_LAYERS[@]}"; do
    export_layer "$KD_KEYMAP" "${LAYOUT_LAYERS[$i]}" "${KEYMAP_NAME}${KEYS_COUNT}-${LAYOUT_MAP_NAMES[$i]}.svg" "$EXTRA_LAYOUT"
  done

  KD_CONFIG_BACKGROUND="$(mktemp).yaml"
  "$PYTHON" "${CONFIG_ROOT}/support/merge_yaml.py" \
    "$KD_CONFIG_MAIN" \
    "${CONFIG_ROOT}/support/keymap-config-background-color.yaml" > "$KD_CONFIG_BACKGROUND"

  echo "- Exporting full layout map image for packaging"
  mkdir -p "${CONFIG_ROOT}/build/out"
  keymap -c "${KD_CONFIG_BACKGROUND}" draw \
    "${EXTRA_FLAG[@]}" \
    -s "${LAYOUT_LAYERS[@]}" \
    -o "${CONFIG_ROOT}/build/out/zmk-${KEYMAP_NAME}$([ -n "${KEYS_COUNT}" ] && echo "-${KEYS_COUNT}")-layout-map.svg" \
    "$KD_KEYMAP"

  rm -rf "$KD_KEYMAP"
  rm -rf "$KD_CONFIG_NO_SYMBOLS"
  rm -rf "$KD_CONFIG_BACKGROUND"

  echo "Done!"
  echo ""
}

export_layout_map Corne 42
export_layout_map Rolio 46 "${CONFIG_ROOT}/support/rolio_layout.json"

echo "Exporting layers Glyphs..."
mkdir -p "${CONFIG_ROOT}/docs/glyphs"

# I need to copy the cached symbols from Keymap Drawer into my repository so
# the table of key symbols I have on the README file can render correctly.
shopt -s nullglob
for g in "$KD_CACHE"/glyphs/*; do
  # I'm using `sed` to copy the file because I need to add `height`, `width`,
  # and `color` to the SVG, since GitHub does not allow me to have all the CSS
  # styles I want... I think :)
  sed 's/viewBox="0 0 24 24"/viewBox="0 0 24 24" height="20px" width="20px" fill="#e8eaed"/' "$g" > "${CONFIG_ROOT}/docs/glyphs/${g##*mdi:}"
done
shopt -u nullglob

echo "Done!"
