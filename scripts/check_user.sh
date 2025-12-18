#!/data/data/com.termux/files/usr/bin/bash

echo "--- User Identity Diagnostics ---"
echo ""

echo "[+] whoami:"
whoami
echo ""

echo "[+] id command:"
id
echo ""

echo "[+] Environment Variables:"
echo "    USER: $USER"
echo "    LOGNAME: $LOGNAME"
echo "    HOME: $HOME"
echo "    SHELL: $SHELL"
echo ""

echo "[+] Current Working Directory:"
pwd
echo ""

echo "--- Diagnostics Complete ---"
