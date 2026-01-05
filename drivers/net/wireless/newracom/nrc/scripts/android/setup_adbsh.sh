#!/bin/bash
################################################################################
# ADB Shell 프롬프트 설정 스크립트 (mobaXterm/Cygwin용)
################################################################################
#
# 용도: Android 단말에 adb shell 접속 시 프롬프트와 tab completion 활성화
#
# 사용법:
#   1. 스크립트 실행: ./setup_adbsh.sh
#   2. bashrc 적용:   source ~/.bashrc
#   3. 접속:          adbsh
#
# 결과:
#   - 프롬프트 표시: rpi4:/ #
#                   rpi4:/data/nrc_pkg/script #
#   - Tab completion 작동 (ls /d[TAB] → ls /data)
#   - 명령 히스토리 (↑↓ 화살표)
#   - Emacs 편집 (Ctrl+A/E/U/K)
#   - Alias: ll, la, l
#
# 생성 파일:
#   Android: /data/local/.mkshrc              - 프롬프트 및 설정
#   Android: /data/local/bin/sh-prompt        - Shell wrapper
#   Host:    ~/.bashrc                        - adbsh 함수
#
################################################################################

set -e

echo "=========================================="
echo "ADB Shell 프롬프트 설정"
echo "=========================================="
echo ""

#--------------------------------------------------
# 1. ADB 연결 확인
#--------------------------------------------------
echo "[1/4] ADB 연결 확인..."
if ! adb devices 2>&1 | grep -q "device$"; then
    echo "    ✗ ADB 장치가 연결되지 않았습니다"
    echo ""
    echo "다음 명령으로 확인하세요:"
    echo "  adb devices"
    exit 1
fi
echo "    ✓ ADB 연결됨"

#--------------------------------------------------
# 2. Android 단말 설정
#--------------------------------------------------
echo ""
echo "[2/4] Android 단말 설정..."

# .mkshrc 파일 생성 (프롬프트, alias, history 설정)
adb shell 'cat > /data/local/.mkshrc << "EOF"
# MKsh 설정 파일
# 프롬프트 형식: HOSTNAME:경로 #

# 호스트명 설정 (Android 모델명 또는 기본값)
HOSTNAME=$(getprop ro.product.model 2>/dev/null || echo rpi4)

# 프롬프트 설정: "rpi4:/data/nrc_pkg #" 형식
PS1="$HOSTNAME:\${PWD} # "
export HOSTNAME PS1

# Emacs 스타일 명령행 편집 활성화 (tab completion 포함)
set -o emacs

# 명령 히스토리 설정
export HISTFILE=/data/local/tmp/.mksh_history
export HISTSIZE=1000

# 편의 Alias
alias ll="ls -la"    # 상세 목록
alias la="ls -A"     # 숨김 파일 포함
alias l="ls -CF"     # 분류 표시
EOF
chmod 644 /data/local/.mkshrc' 2>&1

if [ $? -eq 0 ]; then
    echo "    ✓ .mkshrc 생성 완료"
else
    echo "    ✗ .mkshrc 생성 실패"
    exit 1
fi

# Shell wrapper 스크립트 생성
# ENV를 설정하고 interactive shell 실행
adb shell 'mkdir -p /data/local/bin
cat > /data/local/bin/sh-prompt << "EOF"
#!/system/bin/sh
# Shell wrapper - 프롬프트 설정을 로드하고 interactive shell 시작

export ENV=/data/local/.mkshrc
. /data/local/.mkshrc
exec sh -i
EOF
chmod 755 /data/local/bin/sh-prompt' 2>&1

if [ $? -eq 0 ]; then
    echo "    ✓ Shell wrapper 생성 완료"
else
    echo "    ✗ Shell wrapper 생성 실패"
    exit 1
fi

#--------------------------------------------------
# 3. bashrc 백업 및 업데이트
#--------------------------------------------------
echo ""
echo "[3/4] bashrc 업데이트..."

# 기존 bashrc 백업
if [ -f ~/.bashrc ]; then
    BACKUP_FILE=~/.bashrc.backup.$(date +%Y%m%d_%H%M%S)
    cp ~/.bashrc "$BACKUP_FILE"
    echo "    ✓ 백업: $BACKUP_FILE"
fi

# adbsh 함수가 이미 있는지 확인하고 제거
if [ -f ~/.bashrc ]; then
    grep -v "adbsh" ~/.bashrc > ~/.bashrc.tmp 2>/dev/null || true
    mv ~/.bashrc.tmp ~/.bashrc 2>/dev/null || true
fi

# bashrc에 adbsh 함수 추가
# mobaXterm의 경우 winpty 필요
cat >> ~/.bashrc << 'EOF'

################################################################################
# ADB Shell 함수
################################################################################
adbsh() {
    # mobaXterm/Cygwin: winpty를 사용하여 PTY 할당
    # Linux: 직접 실행
    if command -v winpty >/dev/null 2>&1; then
        winpty adb shell /data/local/bin/sh-prompt
    else
        adb shell /data/local/bin/sh-prompt
    fi
}
EOF

echo "    ✓ bashrc 업데이트 완료"

#--------------------------------------------------
# 4. 설정 검증
#--------------------------------------------------
echo ""
echo "[4/4] 설정 검증..."

# Android 파일 확인
if adb shell "test -f /data/local/.mkshrc && test -x /data/local/bin/sh-prompt" 2>/dev/null; then
    echo "    ✓ Android 파일: OK"
else
    echo "    ✗ Android 파일: 실패"
    exit 1
fi

# bashrc 확인
if grep -q "adbsh()" ~/.bashrc 2>/dev/null; then
    echo "    ✓ bashrc: OK"
else
    echo "    ✗ bashrc: 실패"
    exit 1
fi

#--------------------------------------------------
# 완료 메시지
#--------------------------------------------------
echo ""
echo "=========================================="
echo "✓ 설정 완료!"
echo "=========================================="
echo ""
echo "다음 명령으로 사용하세요:"
echo ""
echo "  1. bashrc 적용:"
echo "     source ~/.bashrc"
echo ""
echo "  2. Android 접속:"
echo "     adbsh"
echo ""
echo "예상 결과:"
echo "  rpi4:/ #"
echo "  rpi4:/data/nrc_pkg/script #"
echo ""
echo "기능:"
echo "  • 프롬프트: 호스트명과 경로 표시"
echo "  • Tab 완성: ls /d[TAB] → ls /data"
echo "  • 히스토리: ↑↓ 화살표로 이전 명령"
echo "  • 편집:    Ctrl+A(처음), E(끝), U(삭제)"
echo "  • Alias:   ll(상세목록), la(숨김포함), l(분류)"
echo ""
echo "문제 발생 시:"
echo "  - 백업 복구: cp ~/.bashrc.backup.* ~/.bashrc"
echo "  - 재실행:    ./setup_adbsh.sh"
echo "=========================================="
