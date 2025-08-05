#!/bin/bash
set -euo pipefail

INSTALL_DIR="${1}"
KEEP_MODULES="${2}"
TMPFILE="$(mktemp)"
FINAL_KEEP="$(mktemp)"

if [ -z "${KEEP_MODULES}" ]; then
  echo "Error: missing keep modules input file" >&2
  exit 1
fi

if [[ ! -f "${KEEP_MODULES}" ]]; then
  echo "Error: keep list file ${KEEP_MODULES} not found." >&2
  exit 1
fi

# determine kernel version
kernel_version=$(ls "${INSTALL_DIR}/modules")
if [ -z "${kernel_version}" ]; then
  echo "Error: could not determine kernel version" >&2
  exit 1
fi

# 1. Resolve dependencies for each module
echo "[+] Resolving dependencies..."

while read -r modname
do
  [[ -z "${modname}" || "${modname}" =~ ^# ]] && continue
  mods=$(modprobe  --dirname "${INSTALL_DIR}" --set-version "${kernel_version}" --show-depends "${modname}" 2>/dev/null || true)
  if [[ -z "${mods}" ]]; then
    echo "    [Missing KO]: ${modname}"
    continue
  fi

  if [[ "${mods}" == "builtin "* && "${mods}" != *$'\n'* ]]; then
    echo "    [Built in KO]: ${modname}"
    continue
  fi

  echo "${mods}" | awk '/insmod/ {print $2}' | while read -r mod
  do
    [ -n "${mod}" ] && realpath "${mod}" >> "${TMPFILE}"
  done
done < "${KEEP_MODULES}"

# 2. Normalise list (absolute paths, real files)
echo "[+] Normalise  dependencies..."
sort -u "${TMPFILE}" > "${FINAL_KEEP}"

# 3. Find all kernel modules
echo "[+] Pruning unused kernel modules from: ${INSTALL_DIR}/modules/${kernel_version}"
find "${INSTALL_DIR}/lib/modules/${kernel_version}" -type f \( -name "*.ko" -o -name "*.ko.zstd" \) | while read -r modfile; do
  kofile=$(realpath "${modfile}")
  if grep -Fxq "${kofile}" "${FINAL_KEEP}"; then
    echo "    [KEEPING KO]: $(basename "${kofile}")"
  else
    # echo "    [RM  ] $(basename "${kofile}")"
    rm -f "${kofile}"
  fi
done

# 4. Clean tree from empty directories
echo "[+] Removing empty directories...."
find "${INSTALL_DIR}/lib/modules/${kernel_version}" -type d -empty -print -delete > /dev/null 2>&1

# 5. Rebuild dependency tree
echo "[+] Running depmod..."
depmod --basedir "${INSTALL_DIR}" "${kernel_version}"

# 6. Cleanup
rm -f "${TMPFILE}" "${FINAL_KEEP}"

echo "[✔] Done pruning with dependencies included."
