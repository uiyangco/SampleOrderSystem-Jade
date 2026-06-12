# PRD — SampleOrderSystem

## 1. 프로젝트 개요

콘솔 기반 시료(Sample) 주문 관리 시스템. 시료 등록·조회·검색, 주문 예약·승인·거절·출고, 생산 라인 관리, 재고 모니터링 기능을 제공한다. 데이터는 JSON 파일로 영속화하며 MVC 패턴을 적용한다.

---

## 2. 도메인 모델

### 2.1 Sample (시료)

| 필드 | 타입 | 설명 |
|------|------|------|
| id | int | 자동 증가 |
| name | wstring | 시료명 |
| avgProductionTime | int | 평균 생산 시간 (분/개) |
| yield | double | 수율 (0.0 ~ 1.0, 예: 0.85 = 85%) |
| stock | int | 현재 재고 수량 |
| createdAt | wstring | 등록일시 (YYYY-MM-DD HH:MM:SS) |

### 2.2 Order (주문)

| 필드 | 타입 | 설명 |
|------|------|------|
| id | int | 자동 증가 |
| sampleId | int | Sample FK |
| customerName | wstring | 고객명 |
| quantity | int | 주문 수량 |
| status | OrderStatus | 주문 상태 (아래 참고) |
| createdAt | wstring | 주문일시 |
| updatedAt | wstring | 최종 상태 변경일시 |

#### 주문 상태 (OrderStatus)

```
RESERVED   → 예약됨 (주문 생성 초기 상태)
    ↓ 승인
CONFIRMED  ← 재고 충분 시
PRODUCING  ← 재고 부족 시 (생산 라인 등록됨)
    ↓ 생산 완료
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
| REJECTED | 거절됨 (이후 처리 없음) |

### 2.3 ProductionJob (생산 작업)

| 필드 | 타입 | 설명 |
|------|------|------|
| id | int | 자동 증가 |
| orderId | int | Order FK |
| sampleId | int | Sample FK |
| targetQty | int | 실 생산 목표량 (`ceil(부족분 / (수율 * 0.9))`) |
| producedQty | int | 현재까지 생산된 수량 |
| totalMinutes | int | 총 생산 소요 시간 (avgProductionTime × targetQty) |
| startedAt | wstring | 생산 시작일시 |
| status | JobStatus | WAITING / RUNNING / DONE |

#### 생산량 계산

```
부족분         = 주문 수량 - 현재 재고
실 생산 목표량  = ceil(부족분 / (수율 * 0.9))
총 생산 시간   = avgProductionTime * 실 생산 목표량  (단위: 분)
```

---

## 3. 기능 명세

### 3.1 시료 관리

메인 메뉴 → **[1] 시료 관리** 진입 후 서브 메뉴 표시.

#### 3.1.1 시료 등록
- 입력값: 이름, 평균 생산시간(분), 수율(%), 초기 재고
- 수율은 0~100 정수로 입력받아 0.0~1.0 double로 저장
- 등록 완료 후 부여된 ID와 함께 확인 메시지 출력

#### 3.1.2 시료 조회
- 등록된 모든 시료를 테이블 형식으로 출력
- 재고 수량을 함께 표기
- 재고 상태 컬럼 포함 (모니터링 섹션의 재고 상태 기준 동일 적용)

#### 3.1.3 시료 검색
- 이름 부분 일치(포함 검색)로 필터링
- 결과가 없을 경우 안내 메시지 출력

---

### 3.2 시료 주문

메인 메뉴 → **[2] 시료 주문** 진입.

- 입력값: 시료 ID, 고객명, 주문 수량
- 존재하지 않는 시료 ID 입력 시 오류 메시지 후 재입력
- 생성된 주문 상태: **RESERVED**
- 생성 완료 후 주문 ID 및 요약 정보 출력

---

### 3.3 주문 승인/거절

메인 메뉴 → **[3] 주문 승인/거절** 진입.

#### 화면 구성
- RESERVED 상태 주문 목록을 먼저 출력
- 처리할 주문 ID 입력 → 승인 또는 거절 선택

#### 승인 로직

```
if (시료 재고 >= 주문 수량)
    재고 차감
    주문 상태 → CONFIRMED

else
    생산 부족분 계산
    ProductionJob 생성 후 생산 큐에 WAITING으로 등록
    주문 상태 → PRODUCING
```

#### 거절 로직
- 주문 상태 즉시 → **REJECTED**
- 재고·생산 라인 변경 없음

---

### 3.4 모니터링

메인 메뉴 → **[4] 모니터링** 진입.

별도 스레드에서 주기적으로 화면을 갱신하는 watch 모드로 동작한다.

#### 주문량 현황
- RESERVED / CONFIRMED / PRODUCING / RELEASE 상태별 주문 건수 및 목록 표기
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

| 상태 | 조건 | 표시 색상 |
|------|------|----------|
| 고갈 | stock == 0 | Red |
| 부족 | stock < 주문 대기량 | Yellow |
| 여유 | stock >= 주문 대기량 | Green |

#### 모니터링 스레드 구성
- **메인 스레드**: 일반 CLI 메뉴 처리
- **모니터링 스레드**: 지정 주기(기본 2000ms)마다 화면 갱신
- 공유 데이터(`DataStore`) 접근 시 `std::mutex` + `std::lock_guard` 동기화
- 주문 상태 변경 시 `OrderRepository`에 등록된 콜백이 `DataStore`에 이벤트 푸시

```cpp
using EventCallback = std::function<void(const OrderEvent&)>;

// OrderController 초기화 시
repo_.setEventCallback([&store](const OrderEvent& e) {
    store.push(e);
});
```

---

### 3.5 출고 처리

메인 메뉴 → **[5] 출고 처리** 진입.

- CONFIRMED 상태 주문 목록 출력
- 처리할 주문 ID 입력
- 해당 주문 상태 → **RELEASE**
- 출고 완료 메시지 출력

---

### 3.6 생산 라인 조회

메인 메뉴 → **[6] 생산 라인 조회** 진입.

#### 현재 생산 현황 (RUNNING)

| 표기 항목 | 설명 |
|----------|------|
| 주문 ID | - |
| 시료명 | - |
| 고객명 | - |
| 목표 생산량 | targetQty |
| 현재 생산량 | producedQty |
| 총 소요 시간 | totalMinutes (분) |
| 생산 시작일시 | startedAt |

#### 생산 대기 큐 (WAITING)
- 생산 큐를 FIFO 순서대로 목록 출력
- 표기 항목: 대기 순번, 주문 ID, 시료명, 고객명, 목표 생산량, 총 소요 시간

#### 생산 완료 처리
- RUNNING 작업이 완료되면:
  1. 시료 재고에 실 생산량 추가
  2. 주문 상태 PRODUCING → **CONFIRMED** 변경
  3. 큐에서 다음 WAITING 작업을 꺼내 RUNNING으로 전환

> **생산 진행 방식: 실시간 타이머**
> - 별도 생산 스레드(`ProductionThread`)가 백그라운드에서 상시 동작
> - RUNNING 작업의 경과 시간을 추적하여 `totalMinutes` 도달 시 자동 완료 처리
> - 완료 시: 재고 추가 → 주문 상태 PRODUCING → CONFIRMED → 다음 WAITING 작업 RUNNING 전환
> - 시간 단위는 실제 분(minute) 기준이나, 테스트 편의를 위해 1분 = N초로 스케일 조정 가능 (설정값으로 분리)

---

## 4. 아키텍처

### 4.1 파일 구조

```
SampleOrderSystem/
├── main.cpp
├── Model/
│   ├── Sample.h / Sample.cpp
│   ├── Order.h / Order.cpp
│   ├── ProductionJob.h / ProductionJob.cpp
│   └── Repository/
│       ├── IRepository.h
│       ├── JsonRepository.h
│       ├── SampleRepository.h/cpp
│       ├── OrderRepository.h/cpp
│       └── ProductionJobRepository.h/cpp
├── View/
│   ├── ConsoleUI.h/cpp
│   ├── SampleView.h/cpp
│   ├── OrderView.h/cpp
│   └── ProductionView.h/cpp
├── Controller/
│   ├── SampleController.h/cpp
│   ├── OrderController.h/cpp
│   ├── ProductionController.h/cpp
│   └── MonitorController.h/cpp
├── Monitor/
│   ├── DataStore.h/cpp
│   └── MonitorLoop.h/cpp
└── json/
    ├── JsonValue.h
    ├── JsonParser.h/cpp
    └── JsonSerializer.h/cpp
```

### 4.2 의존성 방향

```
main
 └─ App (전체 초기화, 메인 루프)
     ├─ SampleController    → SampleRepository → JsonRepository<Sample>
     ├─ OrderController     → OrderRepository  → JsonRepository<Order>
     │                      → SampleRepository (재고 차감)
     │                      → ProductionJobRepository (생산 큐 등록)
     │                      → DataStore (콜백으로 이벤트 푸시)
     ├─ ProductionController→ ProductionJobRepository (조회 전용)
     ├─ MonitorController   → DataStore (읽기)
     ├─ MonitorLoop    [별도 스레드] → DataStore (읽기, 주기적 화면 갱신)
     └─ ProductionThread [별도 스레드] → ProductionJobRepository (RUNNING 작업 추적)
                                      → SampleRepository (재고 추가)
                                      → OrderRepository (상태 변경)
                                      → DataStore (콜백으로 완료 이벤트 푸시)
```

### 4.3 스레드 구성

| 스레드 | 역할 | 주기 |
|--------|------|------|
| 메인 스레드 | CLI 입력 처리, 메뉴 루프 | 블로킹 (사용자 입력 대기) |
| MonitorLoop | 모니터링 화면 주기적 갱신 | 2000ms (기본값) |
| ProductionThread | 생산 타이머 추적, 자동 완료 처리 | 1000ms tick |

- 세 스레드 모두 `DataStore` 및 Repository를 공유하며 `std::mutex`로 동기화
- 프로그램 종료 시 `std::atomic<bool> running_` → false 전환 후 join

---

## 5. 데이터 영속성

DataPersistence POC 구현을 이식한다.

- 저장 형식: JSON 파일 (엔티티 1종당 파일 1개)
- 파일 경로:
  - `data/samples.json`
  - `data/orders.json`
  - `data/production_jobs.json`
- 스키마 구조:

```json
{
  "nextId": 4,
  "data": [
    { "id": 1, "name": "시료A", ... }
  ]
}
```

- 로드: 프로그램 시작 시 전체 로드 (Eager loading)
- 저장: CRUD 연산마다 즉시 전체 덮어쓰기
- 인코딩: UTF-8 (`/utf-8` 컴파일 플래그)
- 트랜잭션: 없음 (각 연산이 독립 저장, 부분 실패 허용)

### IRepository 인터페이스

```cpp
template<typename T>
class IRepository {
    virtual bool             create(T& entity)       = 0;
    virtual std::optional<T> read(int id)            = 0;
    virtual std::vector<T>   readAll()               = 0;
    virtual bool             update(const T& entity) = 0;
    virtual bool             remove(int id)          = 0;
};
```

---

## 6. 콘솔 UI

ConsoleMVC + DataMonitor POC 참고.

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

### 메인 메뉴

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

## 7. 구현 우선순위

| 단계 | 내용 |
|------|------|
| 1 | `json/` 계층 (JsonValue, JsonParser, JsonSerializer) |
| 2 | 도메인 모델 (Sample, Order, ProductionJob) + IRepository |
| 3 | JsonRepository + 파일 I/O |
| 4 | ConsoleUI (색상 출력 유틸) |
| 5 | 시료 관리 (SampleController + SampleView) |
| 6 | 시료 주문 (OrderController 기본 흐름) |
| 7 | 주문 승인/거절 (재고 분기 + 생산 큐 등록) |
| 8 | 출고 처리 |
| 9 | 생산 라인 (ProductionController + 큐 FIFO) |
| 10 | 모니터링 (DataStore + MonitorLoop 스레드) |

---

## 8. 결정 사항 요약

| 항목 | 결정 |
|------|------|
| 데이터 영속성 | JSON 파일 |
| 이벤트 연동 | 콜백 (OrderRepository → DataStore) |
| 트랜잭션 | 없음 |
| 모니터링 | 별도 스레드 (MonitorLoop) |
| 생산 진행 | 실시간 타이머 (ProductionThread, 1000ms tick) |
| 시간 스케일 | 1분 = 설정값 N초 (테스트 편의용 조정 가능) |
