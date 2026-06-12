# CLAUDE.md

Behavioral guidelines to reduce common LLM coding mistakes.
Merge with project-specific instructions as needed.

**Tradeoff:** These guidelines bias toward caution over speed.
For trivial tasks, use judgment.

## 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
-State your assumptions explicitly. If uncertain, ask.
-If multiple interpretations exist, present them - don't pick silently.
-If a simpler approach exists, say so. Push back when warranted.
-If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

-No features beyond what was asked.
-No abstractions for single-use code.
-No "flexibility" or "configurability" that wasn't requested.
-No error handling for impossible scenarios.
-If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
-Don't "improve" adjacent code, comments, or formatting.
-Don't refactor things that aren't broken.
-Match existing style, even if you'd do it differently.
-If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
-Remove imports/variables/functions that YOUR changes made unused.
-Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
-"Add validation" → "Write tests for invalid inputs, then make them pass"
-"Fix the bug" → "Write a test that reproduces it, then make it pass"
-"Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
1.[Step] → verify: [check]
2.[Step] → verify: [check]
3.[Step] → verify: [check]

Strong success criteria let you loop independently.
Weak criteria ("make it work") require constant clarification.

---

## 5. TDD Workflow

**Red → Green → Refactor. Never skip a phase.**

Each feature must follow this cycle strictly:

1. **[RED]** Write a failing test first. Commit before writing production code.
2. **[GREEN]** Write the minimum production code to make the test pass. Commit.
3. **[REFAC]** Clean up without changing behavior. Tests must still pass. Commit.

Rules:
- Never write production code without a failing test that demands it.
- A [GREEN] commit must not introduce new logic beyond what the failing test requires.
- A [REFAC] commit must not change any test expectations.

## 6. Git Commit Rules

Every commit title must be prefixed with the TDD phase:

```
[RED]   <description>    ← failing test added
[GREEN] <description>    ← production code makes test pass
[REFAC] <description>    ← refactoring, no behavior change
```

Examples:
```
[RED]   OrderRepository::create stores order with RESERVED status
[GREEN] OrderRepository::create stores order with RESERVED status
[REFAC] extract status validation into helper
```

- One TDD cycle = three commits minimum (RED → GREEN → REFAC).
- REFAC may be omitted only if there is genuinely nothing to clean up.
- Do not bundle RED + GREEN into a single commit.

## 7. Unit Testing with Google Mock (gmock)

**Framework: GoogleTest + GoogleMock (NuGet)**

- NuGet 패키지: `Microsoft.googletest.v140.windesktop.msvcstl.dyn.rt-dyn` v1.8.1.3
- Test project: `SampleOrderSystem.Tests` (솔루션 내 별도 프로젝트)
  - 프로젝트 파일: `SampleOrderSystem.Tests/SampleOrderSystem.Tests.vcxproj`
  - NuGet 선언: `SampleOrderSystem.Tests/packages.config`
  - Visual Studio에서 솔루션을 열면 NuGet 자동 복원됨
- Test file naming: `<ClassName>Test.cpp` (예: `OrderRepositoryTest.cpp`)
- 테스트 파일은 `SampleOrderSystem.Tests/` 하위에 위치
- 주 프로젝트 소스는 `AdditionalIncludeDirectories`로 참조 (`..\SampleOrderSystem`)

### Mock 작성 규칙

- 인터페이스(`IRepository` 등)에 대해 `MOCK_METHOD`로 mock 클래스 작성
- Mock 클래스명: `Mock<ClassName>` (예: `MockSampleRepository`)
- Mock 파일 위치: `Tests/Mocks/` 하위

```cpp
class MockSampleRepository : public ISampleRepository {
public:
    MOCK_METHOD(bool, create, (Sample&), (override));
    MOCK_METHOD(std::optional<Sample>, read, (int), (override));
    MOCK_METHOD(std::vector<Sample>, readAll, (), (override));
    MOCK_METHOD(bool, update, (const Sample&), (override));
    MOCK_METHOD(bool, remove, (int), (override));
};
```

### 테스트 구조

```cpp
TEST_F(OrderControllerTest, Approve_WhenStockSufficient_SetsConfirmed) {
    // Arrange
    EXPECT_CALL(mockOrderRepo_, update(_))
        .WillOnce(Return(true));

    // Act
    controller_.approve(orderId);

    // Assert
    EXPECT_EQ(order_.status, OrderStatus::CONFIRMED);
}
```

- Arrange / Act / Assert 구조를 명시적으로 유지한다.
- 각 테스트는 하나의 동작만 검증한다.