# NRC Driver Memory Debugging Tools

kmalloc-512 메모리 릭 추적 및 모니터링 도구 모음입니다.

## 빠른 시작

### 1. 실시간 메모리 모니터링
```bash
# 2초 간격으로 모니터링
./tools/monitor_memory.sh

# 5초 간격으로 모니터링
./tools/monitor_memory.sh 5
```

### 2. 모듈 로드/언로드 전후 비교
```bash
./tools/compare_memory.sh
```

이 스크립트는 다음을 자동으로 수행합니다:
1. 모듈 로드 전 메모리 스냅샷
2. 모듈 로드 후 메모리 스냅샷
3. 모듈 언로드 후 메모리 스냅샷
4. 메모리 릭 자동 감지

### 3. 코드 분석
```bash
./tools/analyze_allocations.sh
```

드라이버 소스 코드를 분석하여:
- 메모리 할당 패턴 파악
- 잠재적 릭 위치 식별
- SKB 할당/해제 균형 확인

## 상세 사용법

### Monitor Memory (monitor_memory.sh)

실시간으로 SLAB 메모리 사용량을 모니터링합니다.

**표시 정보:**
- kmalloc-512 캐시 상태 (증가/감소 추적)
- 다른 kmalloc 캐시들
- SKB 버퍼 사용량
- NRC 모듈 로드 상태

**사용 예:**
```bash
# 실시간 모니터링 시작
./tools/monitor_memory.sh

# 모듈 로드 전 실행하고, 다른 터미널에서 모듈 로드
# 메모리 증가를 실시간으로 확인
```

### Compare Memory (compare_memory.sh)

모듈 로드/언로드 전후의 메모리 차이를 분석합니다.

**워크플로우:**
1. 스크립트 실행
2. 프롬프트에 따라 모듈 로드
3. 메모리 차이 확인
4. 프롬프트에 따라 모듈 언로드
5. 메모리 릭 자동 감지

**출력:**
- 모듈 로드로 인한 메모리 증가
- kmalloc-512 특정 추적
- 언로드 후 남은 메모리 (릭 의심)

### Analyze Allocations (analyze_allocations.sh)

소스 코드를 정적 분석하여 메모리 할당 패턴을 파악합니다.

**분석 항목:**
- 파일별 kmalloc/kzalloc 호출 횟수
- SKB 할당/해제 비율
- 워크큐 생성/파괴 비율
- 타이머 초기화/삭제 비율
- 512바이트 할당 패턴

## 고급 사용법

### KMEMLEAK 사용

```bash
# kmemleak 활성화 확인
zcat /proc/config.gz | grep CONFIG_DEBUG_KMEMLEAK

# 모듈 로드 전 초기화
echo scan > /sys/kernel/debug/kmemleak

# 모듈 로드 및 테스트
insmod nrc_spi.ko
insmod nrc_core.ko
insmod nrc_wlan.ko
# ... 네트워크 트래픽 발생 ...

# 릭 스캔
echo scan > /sys/kernel/debug/kmemleak
cat /sys/kernel/debug/kmemleak

# 모듈 언로드
rmmod nrc_wlan nrc_core nrc_spi

# 최종 스캔
echo scan > /sys/kernel/debug/kmemleak
cat /sys/kernel/debug/kmemleak
```

### SLUB 디버깅

```bash
# kmalloc-512 캐시 추적 활성화
echo 1 > /sys/kernel/slab/kmalloc-512/trace

# 할당 추적
cat /sys/kernel/slab/kmalloc-512/alloc_calls

# 해제 추적
cat /sys/kernel/slab/kmalloc-512/free_calls

# 현재 객체 수
cat /sys/kernel/slab/kmalloc-512/objects
```

### 수동 메모리 비교

```bash
# Before snapshot
cat /proc/slabinfo > before.txt

# 모듈 로드 및 테스트
./start_wlan.py 0 0 US
# ... 테스트 ...
./stop.py

# After snapshot
cat /proc/slabinfo > after.txt

# 비교
diff before.txt after.txt | grep "^[<>]" | grep kmalloc-512
```

## 일반적인 메모리 릭 원인

### 1. SKB 릭
```c
// 문제: 에러 발생 시 SKB 해제하지 않음
skb = alloc_skb(size, GFP_KERNEL);
if (error_condition)
    return -ERROR;  // SKB 릭!

// 해결: 에러 경로에서 SKB 해제
skb = alloc_skb(size, GFP_KERNEL);
if (!skb)
    return -ENOMEM;
if (error_condition) {
    dev_kfree_skb(skb);
    return -ERROR;
}
```

### 2. 워크큐 릭
```c
// 모듈 언로드 시 워크큐 정리 필요
cancel_work_sync(&work);
flush_workqueue(wq);
destroy_workqueue(wq);
```

### 3. 타이머 릭
```c
// 모듈 언로드 시 타이머 삭제 필요
del_timer_sync(&timer);
// 또는
hrtimer_cancel(&hrtimer);
```

### 4. Completion 릭
```c
// wait_for_completion_timeout() 사용 후
// timeout 발생 시에도 리소스 정리 필요
ret = wait_for_completion_timeout(&done, timeout);
if (ret == 0) {
    // Timeout - cleanup needed!
}
```

## 트러블슈팅

### kmalloc-512가 계속 증가하는 경우

1. **실시간 모니터링으로 증가 시점 파악**
   ```bash
   ./tools/monitor_memory.sh
   ```

2. **SLUB 추적으로 할당 위치 확인**
   ```bash
   cat /sys/kernel/slab/kmalloc-512/alloc_calls | sort -rn | head -20
   ```

3. **모듈 언로드 후 메모리 확인**
   ```bash
   ./tools/compare_memory.sh
   ```

4. **KMEMLEAK으로 정확한 릭 위치 파악**
   ```bash
   echo scan > /sys/kernel/debug/kmemleak
   cat /sys/kernel/debug/kmemleak
   ```

### 도구가 작동하지 않는 경우

- **bc 명령어 설치 필요**
  ```bash
  sudo apt-get install bc
  ```

- **/proc/slabinfo 권한 확인**
  ```bash
  sudo cat /proc/slabinfo  # root 권한 필요
  ```

- **debugfs 마운트 확인**
  ```bash
  mount | grep debugfs
  # 없으면:
  sudo mount -t debugfs none /sys/kernel/debug
  ```

## 참고 자료

- 상세 가이드: `docs/MEMORY_DEBUGGING.md`
- 커널 문서: `/usr/src/linux/Documentation/vm/`
- SLUB 디버깅: `/usr/src/linux/Documentation/vm/slub.txt`
- KMEMLEAK: `/usr/src/linux/Documentation/dev-tools/kmemleak.rst`

## 문제 보고

메모리 릭을 발견한 경우:

1. `compare_memory.sh` 출력 저장
2. `dmesg` 로그 수집
3. 재현 단계 기록
4. GitHub Issue 생성

## 라이센스

NRC Driver와 동일한 라이센스 적용
