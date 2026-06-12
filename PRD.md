# PRD — SampleOrderSystem

## 1. 프로젝트 개요

콘솔 기반 시료(Sample) 주문 관리 시스템. 시료 등록·조회·검색·제거, 주문 예약·승인·거절·출고, 생산 라인 관리, 재고 모니터링 기능을 제공한다. 데이터는 JSON 파일로 영속화하며 MVC 패턴을 적용한다.

---

## 2. 도메인 모델

### 2.1 Sample (시료)

| 필드 | 타입 | 설명 |
|------|------|------|
| id | int | 자동 증가 |
| name | wstring | 시료명 (중복 불가) |
| avgProductionTime | int | 평균 생산 시간 (분/개) |
| yield | double | 수율 (0.0 ~ 1.0, 예: 0.92) |
| stock | int | 현재 재고 수량 |
| createdAt | wstring | 등록일시 (YYYY-MM-DD HH:MM:SS) |

### 2.2 Order (주문)

| 필드 | 타입 | 설명 |
|------|------|------|
| id | int | 자동 증가 |
| sampleId | int | Sample FK |
| customerName | wstring | 고객명 (빈 값 불가) |
| quantity | int | 주문 수량 |
| status | OrderStatus | 주문 상태 (아래 참고) |
| createdAt | wstring | 주문일시 |
| updatedAt | wstring | 최종 상태 변경일시 |

#### 주문 상태 (OrderStatus)

```
RESERVED   → 예약됨 (주문 생성 초기 상태)
    ↓ 승인
CONFIRMED  ← 재고 충분 시 (부족분 = 0)
PRODUCING  ← 재고 부족 시 (ProductionJob 생성 → 생산 큐 WAITING 등록)
    ↓ 생산 완료 (ProductionThread 자동 처리)
CONFIRMED
    ↓ 출고 처리
RELEASE

RESERVED → REJECTED  (거절)
```

| 상태 | 설명 |
|------|------|
| RESERVED | 주문 접수, 승인 대기 중 |
| CONFIRMED | 재고 확보 완료, 출고 대기 |
| PRODUCING | 생산 라인에 등록되어 생산 진행 중 |
| RELEASE | 출고 완료 |
| REJECTED | 거절됨 |

### 2.3 ProductionJob (생산 작업)

| 필드 | 타입 | 설명 |
|------|------|------|
| id | int | 자동 증가 |
| orderId | int | Order FK |
| sampleId | int | Sample FK |
| shortage | int | 부족분 (승인 시 계산) |
| targetQty | int | 실 생산 목표량 (`ceil(부족분 / (수율 × 0.9))`) |
| producedQty | int | 현재까지 생산된 수량 (tick마다 갱신, JSON 영속화) |
| totalMinutes | int | 총 생산 소요 시간 (avgProductionTime × targetQty) |
| startedAtMs | int64 | 생산 시작 Unix 타임스탬프 (ms), double로 직렬화 |
| startedAt | wstring | 생산 시작일시 (표시용) |
| status | JobStatus | WAITING / RUNNING / DONE |

#### 생산량 계산

```
유효 재고    = 현재재고 - CONFIRMED 주문량 합계 - PRODUCING 주문량 합계
부족분       = max(0, 주문수량 - max(0, 유효재고))
실 생산량    = ceil(부족분 / (수율 × 0.9))   (수율 = 0이면 부족분과 동일)
총 생산시간  = avgProductionTime × 실 생산량  (단위: 분)
```

---

## 3. 기능 명세

### 3.1 시료 관리

메인 메뉴 → **[1] 시료 관리** 진입.

#### 서브 메뉴
```
1. 시료 등록
2. 시료 조회
3. 시료 검색
4. 시료 제거
0. 뒤로
```

#### 3.1.1 시료 등록
- 입력값: 이름, 평균 생산시간(분/개), 수율(0.0~1.0), 초기 재고
- **시료명 빈 값 입력 불가** (재입력 요구)
- **동일 시료명 중복 등록 불가** (오류 메시지 후 취소)
- 수율: 0.0~1.0 범위 double 직접 입력 (예: 0.92)
- 등록 완료 후 부여된 ID와 함께 확인 메시지 출력

#### 3.1.2 시료 조회
- 등록된 모든 시료를 테이블 형식으로 출력
- 재고 수량 및 재고 상태 함께 표기

#### 3.1.3 시료 검색
- 이름 부분 일치(포함 검색)로 필터링
- 결과가 없을 경우 안내 메시지 출력

#### 3.1.4 시료 제거
- 시료 목록 출력 후 삭제할 ID 입력
- 삭제 전 시료명·재고 확인 및 y/n 재확인
- 0 입력 시 취소(뒤로)

---

### 3.2 시료 주문

메인 메뉴 → **[2] 시료 주문** 진입.

- 입력값: 시료 ID, 고객명, 주문 수량
- **고객명 빈 값 입력 불가** (재입력 요구)
- 존재하지 않는 시료 ID 입력 시 오류 메시지
- 생성된 주문 상태: **RESERVED**
- 생성 완료 후 주문 ID 및 요약 정보 출력

---

### 3.3 주문 승인/거절

메인 메뉴 → **[3] 주문 승인/거절** 진입.

#### 화면 구성
- RESERVED 상태 주문 목록 출력
- 처리할 주문 ID 입력 → 승인(1) 또는 거절(2) 선택

#### 승인 로직

```
유효재고 = 현재재고 - CONFIRMED 주문량 합계 - PRODUCING 주문량 합계

if 부족분 == 0:
    주문 상태 → CONFIRMED  (즉시, 생산 잡 생성 없음)

else:
    실생산량 = ceil(부족분 / (수율 × 0.9))
    ProductionJob 생성 (status=WAITING)
    주문 상태 → PRODUCING
```

> **주의**: 유효재고 계산 시 RUNNING 잡의 예정 생산량은 제외한다. 재고는 생산 tick마다 실시간으로 직접 stock에 반영되므로, 현재 stock 값이 이미 지금까지 생산된 수량을 포함한다.

#### 거절 로직
- 주문 상태 즉시 → **REJECTED**
- 재고·생산 라인 변경 없음

---

### 3.4 모니터링

메인 메뉴 → **[4] 모니터링** 진입.

별도 watch 스레드에서 1초 주기로 화면을 갱신. 엔터 입력 시 종료.

#### 주문량 현황
- RESERVED / CONFIRMED / PRODUCING / RELEASE 상태별 주문 건수 및 목록
- REJECTED는 표시하지 않음

#### 재고 현황

각 시료별로 아래 정보를 표기한다.

| 컬럼 | 설명 |
|------|------|
| 시료명 | - |
| 현재 재고 | stock |
| 주문 대기량 | RESERVED + CONFIRMED + PRODUCING 상태 주문의 수량 합계 |
| 재고 상태 | 아래 기준 적용 |

**재고 상태 기준**

| 상태 | 조건 | 색상 |
|------|------|------|
| 고갈 | stock == 0 | Red |
| 부족 | stock < 주문 대기량 | Yellow |
| 여유 | stock >= 주문 대기량 | Green |

---

### 3.5 출고 처리

메인 메뉴 → **[5] 출고 처리** 진입.

- CONFIRMED 상태 주문 목록 출력
- 처리할 주문 ID 입력
- 주문 상태 → **RELEASE**, 재고 차감

---

### 3.6 생산 라인 조회

메인 메뉴 → **[6] 생산 라인 조회** 진입.

1초 주기 실시간 갱신. 엔터 입력 시 종료.

#### 화면 상단
- 현재 시각 (HH:MM:SS) 표시

#### 현재 생산 중 (RUNNING)

| 표기 항목 | 설명 |
|----------|------|
| 주문 ID | - |
| 시료명 | - |
| 고객명 | - |
| 주문량 | quantity |
| 부족분 | shortage |
| 실생산량 | targetQty |
| 진행률 바 | `[████████░░░░]  40%` |
| 완료 예정 | `startedAtMs + totalMinutes × timeScale` 기준 시각 (HH:MM:SS) |

#### 생산 대기 큐 (WAITING, FIFO 순)

| 표기 항목 | 설명 |
|----------|------|
| 대기 순번 | - |
| 주문 ID | - |
| 시료명 | - |
| 고객명 | - |
| 주문량 | quantity |
| 부족분 | shortage |
| 실생산량 | targetQty |
| 총시간 | totalMinutes (분) |
| 완료 예정 | 앞 작업 완료 시각 기준 순차 계산 (HH:MM:SS) |

#### 생산 완료 처리 (ProductionThread 자동)
1. tick마다 경과 시간 → `producedQty` 갱신 → 재고 즉시 반영
2. `producedQty >= targetQty` 도달 시:
   - 주문 상태 PRODUCING → **CONFIRMED**
   - 잡 상태 RUNNING → **DONE**
   - 다음 WAITING 잡 → **RUNNING** 전환

---

## 4. 아키텍처

### 4.1 파일 구조

```
SampleOrderSystem/
├── main.cpp                     (kTimeScaleSecPerMin 설정값 포함)
├── Model/
│   ├── Sample.h / Sample.cpp
│   ├── Order.h / Order.cpp
│   ├── ProductionJob.h / ProductionJob.cpp
│   └── Repository/
│       ├── IRepository.h
│       └── JsonRepository.h     (파일 기반 제네릭 구현)
├── View/
│   ├── ConsoleUI.h/cpp          (색상 출력, 입력 유틸)
│   ├── SampleView.h/cpp
│   ├── OrderView.h/cpp
│   ├── ProductionView.h/cpp
│   └── MonitorView.h/cpp
├── Controller/
│   ├── SampleController.h/cpp
│   ├── OrderController.h/cpp
│   ├── ProductionController.h/cpp
│   └── MonitorController.h/cpp
├── Monitor/
│   ├── ProductionThread.h/cpp   (생산 타이머 스레드)
│   └── ProductionCalc.h/cpp     (calcProducedQty 순수 계산)
├── data/
│   ├── samples.json
│   ├── orders.json
│   └── production_jobs.json
└── json/
    ├── JsonValue.h
    ├── JsonParser.h/cpp
    └── JsonSerializer.h/cpp
```

### 4.2 의존성 방향

```
main
 ├─ SampleController    → IRepository<Sample>
 ├─ OrderController     → IRepository<Order>
 │                      → IRepository<Sample>  (유효재고 계산)
 │                      → IRepository<ProductionJob>  (잡 생성)
 ├─ ProductionController→ IRepository<ProductionJob> / Order / Sample  (조회 전용)
 ├─ MonitorController   → IRepository<Order> / Sample  (조회 전용)
 └─ ProductionThread    → IRepository<ProductionJob>  (tick, 갱신)
    [백그라운드 스레드]  → IRepository<Sample>  (재고 증가)
                        → IRepository<Order>   (상태 CONFIRMED 전환)
```

### 4.3 스레드 구성

| 스레드 | 역할 | 주기 |
|--------|------|------|
| 메인 스레드 | CLI 입력, 메뉴 루프 | 블로킹 (사용자 입력 대기) |
| watch 스레드 (모니터링) | 화면 주기적 갱신, 엔터로 종료 | 1000ms |
| watch 스레드 (생산라인) | 화면 주기적 갱신, 엔터로 종료 | 1000ms |
| ProductionThread | 생산 tick, 재고·상태 자동 갱신 | 1000ms |

- `IRepository` 파일 I/O는 `ProductionThread::mutex_`로 직렬화
- `std::atomic<bool> running_`으로 스레드 생명주기 제어

---

## 5. 데이터 영속성

- 저장 형식: JSON 파일 (엔티티 1종당 파일 1개)
- 파일 경로: `data/samples.json`, `data/orders.json`, `data/production_jobs.json`
- 스키마:

```json
{
  "nextId": 4,
  "data": [
    { "id": 1, "name": "시료A", ... }
  ]
}
```

- 로드: 각 CRUD 호출마다 파일 전체 읽기 (read-through)
- 저장: CRUD 연산마다 즉시 전체 덮어쓰기
- 인코딩: UTF-8 (`/utf-8` 컴파일 플래그)
- `startedAtMs`: int64 값을 double로 직렬화 (int32 오버플로우 방지)

#### 앱 재시작 시 복원 동작

| 잡 상태 | 복원 방법 |
|---------|----------|
| RUNNING | `adjustStartedAtForResume()`: `startedAtMs = nowMs() - (producedQty × perUnitMin × timeScale × 1000)` 으로 재조정 → 생산 이어서 진행 |
| WAITING | JSON에 그대로 보존 → `startNextWaiting()`이 자연스럽게 이어받음 |
| DONE | 변경 없음 |

---

## 6. 콘솔 UI

- 문자셋: `std::wstring` / `std::wcin` / `std::wcout`
- 폰트: 시작 시 Malgun Gothic 설정
- 스트림 모드: `_setmode(_fileno(stdout), _O_U16TEXT)`
- 색상: Windows Console API (`SetConsoleTextAttribute`)

| 용도 | 색상 |
|------|------|
| 헤더, 타이틀 | Cyan |
| 정상 / 여유 | Green |
| 경고 / 부족 | Yellow |
| 오류 / 고갈 | Red |
| 일반 텍스트 | White |
| 보조 텍스트, 구분선 | Gray |

#### 입력 유틸 (ConsoleUI)

| 메서드 | 설명 |
|--------|------|
| `inputInt(prompt)` | 정수 입력 |
| `inputLine(prompt)` | 문자열 입력 (빈 값 허용) |
| `inputNonEmptyLine(prompt)` | 빈 값 입력 시 재요청 |
| `inputDouble(prompt, min, max)` | 범위 검증 double 입력, 범위 벗어나면 재요청 |

#### 메인 메뉴

```
════════════════════════════════════
  SampleOrderSystem v1.0
════════════════════════════════════
1. 시료 관리
2. 시료 주문
3. 주문 승인/거절
4. 모니터링
5. 출고 처리
6. 생산 라인 조회
0. 종료
```

---

## 7. 생산 시간 스케일 설정

`main.cpp`의 `kTimeScaleSecPerMin` 값으로 조정:

| 값 | 의미 |
|----|------|
| `1.0` | 생산시간 1분 = 실제 1초 (기본, 빠른 테스트용) |
| `60.0` | 생산시간 1분 = 실제 60초 (실제 시간) |

---

## 8. 구현 현황

| 항목 | 상태 | 비고 |
|------|------|------|
| JSON 계층 | ✅ 완료 | JsonValue / JsonParser / JsonSerializer |
| 도메인 모델 | ✅ 완료 | Sample, Order, ProductionJob |
| IRepository / JsonRepository | ✅ 완료 | 파일 기반 제네릭 |
| ConsoleUI 입력 유틸 | ✅ 완료 | inputNonEmptyLine, inputDouble |
| 시료 관리 | ✅ 완료 | 등록/조회/검색/제거, 중복명 방지, 빈값 방지 |
| 시료 주문 | ✅ 완료 | 고객명 빈값 방지 |
| 주문 승인/거절 | ✅ 완료 | 유효재고 기반 부족분 계산, 즉시 CONFIRMED |
| 출고 처리 | ✅ 완료 | |
| 생산 스레드 | ✅ 완료 | 실시간 tick, 재고 즉시 반영, 재시작 복원 |
| 생산 라인 조회 | ✅ 완료 | 실시간 갱신, 완료예정 시각, 현재시각 |
| 모니터링 | ✅ 완료 | 실시간 갱신, 재고 상태 색상 |
| 앱 재시작 복원 | ✅ 완료 | RUNNING 잡 startedAtMs 재조정, WAITING 자동 복원 |
