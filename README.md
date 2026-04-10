[← Week 10][link-week10] | [Week 12 →][link-week12]

![preview][img-preview]

# DX-Engine — Week 11: Animation Node Editor

> UE5 Blueprint를 레퍼런스로 설계한 노드 기반 애니메이션 편집기 — 코드 없이 노드 연결만으로 애니메이션 상태 머신을 구성하고 즉시 실행 결과를 확인

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![ImGui](https://img.shields.io/badge/ImGui-Node_Editor-FF6F00)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)

---

## Features

- **노드 기반 비주얼 스크립팅** — UE5 Blueprint를 레퍼런스로, 드래그-앤-드롭만으로 애니메이션 상태와 전이 조건을 구성. 재빌드 없이 실행만으로 즉시 결과 확인
- **Expression Tree 평가 엔진** — 노드 그래프를 AST로, `EvaluateInput<T>`를 재귀 eval 함수로 설계. 컴파일러 이론의 Tree-Walk Interpreter 패턴을 그대로 적용
- **3단계 AnimGraph 컴파일러** — 시각적 그래프를 `FAnimationState` + `std::function<bool()>` 람다 클로저로 변환. `BeginPlay` 1회 컴파일 후 매 틱은 상태 머신만 구동

---

## Key Systems

### 1. AnimGraph 편집기와 컴파일 파이프라인

![editor][img-editor]

기존 방식에서는 전이 조건을 변경할 때마다 엔진 코어 C++를 직접 수정하고 재빌드해야 했다. 엔진 코드와 게임 로직의 경계가 불분명했고, 조건 하나를 바꾸기 위해 빌드를 반복해야 했다.

`UAnimationStateMachine`이 `std::function<bool()>`과 `IAnimPoseProvider*`를 인터페이스로 사용하도록 설계되어 있었기 때문에, 기존 코드를 전혀 건드리지 않고 편집기 레이어를 추가할 수 있었다. 이 두 인터페이스가 **편집기와 런타임 사이의 유일한 경계**다.

```
엔진 코어와 게임 로직 사이에 명확한 경계를 만들고,
전이 조건 편집을 코드 밖으로 꺼낸 것
```

`UAnimationGraph`는 `UEdGraph`를 상속해 노드 배치·핀 연결·직렬화를 그대로 활용하는 애니메이션 전용 그래프다. 에디터에서 편집한 그래프는 `BeginPlay`에서 `FAnimBlueprintCompiler::Compile()`을 통해 단 한 번 런타임 구조로 변환된다. 이후 매 틱에서는 이미 컴파일된 `UAnimationStateMachine`만 구동한다.

```
UAnimationGraph          FAnimBlueprintCompiler        UAnimationStateMachine
(UEdGraph 상속)           ::Compile()                   (기존 시스템)
                          ─────────────────
노드 + 핀 + 연결          Stage 1: AnimState 등록        FAnimationState
에디터 전용 데이터   →    Stage 2: 진입점 설정      →    IAnimPoseProvider*
(런타임에 미존재)          Stage 3: 전이 람다 생성        std::function<bool()>
```

<details>
<summary><b>컴파일러 3단계 상세 — 클릭해서 펼치기</b></summary>

<br>

**Stage 1 — AnimState 등록**

`UK2Node_AnimState` 노드를 순회하며 `FAnimationState`를 생성한다. `Animation` 핀 평가 결과가 `UAnimSequence*`이면 직접 연결하고, `UBlendSpace1D*`이면 `IAnimPoseProvider`로 래핑하면서 매 틱 파라미터를 갱신하는 `OnUpdate` 람다를 함께 캡처한다.

```cpp
if (std::holds_alternative<UAnimSequence*>(AnimValue)) {
    AnimState.PoseProvider = Cast<UAnimSequence>(...);
} else {
    AnimState.PoseProvider = BlendSpaceNode->GetBlendSpace();
    AnimState.OnUpdate = [BlendSpaceNode, InAnimInstance]() {
        float Speed = FBlueprintEvaluator::EvaluateInput<float>(
            BlendSpaceNode->FindPin("Speed"), &RuntimeContext);
        BlendSpaceNode->SetParameter(Speed);  // 매 틱 파라미터 갱신
    };
}
OutStateMachine->AddState(AnimState);
```

**Stage 2 — 진입점 설정**

`UK2Node_AnimStateEntry` 노드를 탐색해 연결된 첫 번째 AnimState를 초기 상태로 등록한다.

```cpp
OutStateMachine->SetInitialState(FirstStateName);
```

**Stage 3 — 전이 조건 람다 생성**

`UK2Node_AnimTransition` 노드를 순회하며 From/To 상태 이름을 결정하고, `Can Transition` 핀에 연결된 Expression Tree를 캡처한 람다 클로저를 생성한다. `Condition()`이 호출될 때마다 그래프를 재귀 탐색해 런타임 값을 반환한다.

```cpp
auto Condition = [TransitionNode, InAnimInstance]() -> bool {
    FBlueprintContext RuntimeContext(InAnimInstance);
    return FBlueprintEvaluator::EvaluateInput<bool>(
        TransitionNode->FindPin("Can Transition"), &RuntimeContext);
};
OutStateMachine->AddTransition({ FromName, ToName, Condition, BlendTime });
```

전이 조건은 컴파일 결과물이 아니라 **원본 그래프를 캡처한 람다** — 런타임에 `Condition()`이 호출될 때마다 그래프를 재귀 탐색해 그 시점의 AnimInstance 상태를 읽는다. 바이너리나 바이트코드를 생성하는 전통적인 컴파일과 다르다.

</details>

---

### 2. Expression 노드 평가 엔진

![expression][img-expression]

노드 그래프는 **Expression Tree**다. Statement 없이 값을 반환하는 Expression으로만 구성되어 있으며, 컴파일러 이론의 Tree-Walk Interpreter 패턴을 그대로 적용했다.

| 이 시스템 | PL 이론 대응 |
|---|---|
| `UEdGraph` | AST (Abstract Syntax Tree) |
| `UEdGraphPin.LinkedTo[]` | AST 자식 노드 포인터 |
| `EvaluateInput<T>(pin, ρ)` | `eval(expr, ρ)` |
| `FBlueprintContext` | 평가 환경 ρ |
| `FBlueprintValue` | Sum Type (값 타입) |

`EvaluateInput<T>`는 재귀 eval 함수다. 연결이 있으면 연결된 노드로 이동해 재귀 평가하고, 연결이 없으면 `DefaultValue`를 리터럴로 반환한다. `FBlueprintContext`(평가 환경 ρ)는 재귀 전 구간에 그대로 전달된다.

```cpp
// ρ ⊢ Literal(v) ⇒ v
// ρ ⊢ Greater(e₁, e₂) ⇒ (v₁ > v₂)  where ρ ⊢ e₁ ⇒ v₁,  ρ ⊢ e₂ ⇒ v₂
template <typename T>
T EvaluateInput(UEdGraphPin* pin, FBlueprintContext* ρ) {
    if (pin->LinkedTo.Num() > 0) {
        UEdGraphPin* src = pin->LinkedTo[0];
        return src->OwningNode->EvaluatePin(src, ρ).Get<T>(); // 재귀
    }
    return ParseString<T>(pin->DefaultValue); // ρ ⊢ Literal(v) ⇒ v
}

FBlueprintValue FloatGreater::EvaluatePin(UEdGraphPin* pin, FBlueprintContext* ρ) {
    float a = EvaluateInput<float>(FindPin("A"), ρ); // ρ ⊢ e₁ ⇒ v₁
    float b = EvaluateInput<float>(FindPin("B"), ρ); // ρ ⊢ e₂ ⇒ v₂
    return a > b;
}
```

`FBlueprintValue = std::variant<int32, float, bool, UAnimSequence*>`는 표현 가능한 타입을 컴파일 타임에 고정하는 Sum Type이다. `std::any` 대비 런타임 타입 소거 없이 `Get<T>()`로 안전하게 값을 추출하며, 목록 밖의 타입은 컴파일 타임에 차단된다.

<details>
<summary><b>구현 상세 — 클릭해서 펼치기</b></summary>

<br>

**EvaluateBinaryOp — STL 함수 객체로 12종 노드를 1개 템플릿으로**

수학 연산 노드 12종(`FloatAdd`, `IntMultiply`, `FloatGreater`, ...)은 모두 `EvaluateBinaryOp` 템플릿 하나로 수렴한다. STL 함수 객체(`std::plus`, `std::multiplies`, `std::greater` 등)를 파라미터로 주입해 각 노드 구현을 1줄로 압축했다.

```cpp
template <typename T, typename OpFunc>
FBlueprintValue EvaluateBinaryOp(const UEdGraphNode* Node, OpFunc Op, FBlueprintContext* ρ) {
    T A = EvaluateInput<T>(Node->FindPin("A"), ρ);
    T B = EvaluateInput<T>(Node->FindPin("B"), ρ);
    return FBlueprintValue(Op(A, B));
}

// 각 노드 구현 — 1줄
FBlueprintValue UK2Node_FloatAdd::EvaluatePin(...) {
    return EvaluateBinaryOp<float>(this, std::plus<float>{}, Context);
}
FBlueprintValue UK2Node_FloatGreater::EvaluatePin(...) {
    return EvaluateBinaryOp<float>(this, std::greater<float>{}, Context);
}
```

**EvaluateSelectOp — Short-circuit Evaluation**

Select 노드는 조건에 따라 True/False 브랜치 중 하나만 평가한다. 실행하지 않는 브랜치는 재귀 진입 자체를 하지 않는다. C++의 `&&`, `||` 연산자와 동일한 단락 평가(Short-circuit Evaluation) 의미론이다.

```cpp
bool Cond = EvaluateInput<bool>(FindPin("Condition"), ρ);
return Cond
    ? EvaluateInput<T>(FindPin("True"),  ρ)  // True 브랜치만 평가
    : EvaluateInput<T>(FindPin("False"), ρ); // False 브랜치만 평가
```

**OCP — ObjectFactory 노드 자동 발견**

새 노드 타입은 `ObjectFactory`에 등록하고 `GetMenuActions()`만 구현하면 컨텍스트 메뉴에 자동 등록된다. `FBlueprintActionDatabase`가 초기화 시 레지스트리를 순회하며 `UK2Node` 파생 클래스를 발견한다.

```cpp
for (auto [Class, Func] : ObjectFactory::GetRegistry()) {
    if (Class->IsChildOf(UK2Node::StaticClass())) {
        UK2Node* Node = Cast<UK2Node>(ConstructObject(Class));
        Node->GetMenuActions(Registrar); // 새 노드 타입 추가 시 이것만 구현
        DeleteObject(Node);
    }
}
```

</details>

---

### 3. 애니메이션 노드 시스템

![animation][img-animation]

애니메이션 노드는 Expression 노드와 달리 값을 반환하지 않는 **선언(Declaration)** 에 가깝다. 상태 머신의 구조를 정의하고, 컴파일 시 런타임 객체로 변환된다.

**AnimState** — 하나의 애니메이션 상태를 정의한다. `Animation` 핀에 `AnimSequence` 또는 `BlendSpace1D`를 연결할 수 있다. 두 타입 모두 `IAnimPoseProvider`를 구현하므로 상태 머신이 구체 타입을 알 필요가 없다.

**AnimTransition** — 두 상태 사이의 전이를 정의한다. `Can Transition` 핀에 Expression Tree를 연결하면 컴파일 시 `std::function<bool()>` 람다 클로저로 변환된다. 런타임에 매 틱 조건을 평가해 전이 여부를 결정한다.

**BlendSpace1D** — float 파라미터로 두 애니메이션 클립을 선형 보간한다. `Speed` 핀에도 Expression Tree가 연결되어 `OnUpdate` 람다가 매 틱 `EvaluateInput<float>`으로 값을 읽어 블렌딩 파라미터를 갱신한다.

```
AnimTransition                      BlendSpace1D
Can Transition ──▶ [FloatGreater]   Speed ──▶ [GetVelocity]
                       ├─ [GetVelocity]
                       └─ [Literal 200]
       ↓ std::function<bool()>              ↓ std::function<void()>
 런타임: Condition() 호출            런타임: OnUpdate() 호출
 → AST 재귀 탐색 → true/false        → AST 재귀 탐색 → SetParameter(speed)
```

두 핀 모두 동일한 `EvaluateInput` 메커니즘으로 Expression Tree를 탐색한다. 차이는 타입(`bool` vs `float`)과 호출 시점(`EvaluateTransitions` vs `OnUpdate`)뿐이다.

---

### 4. 실제 사용 사례 — Sky Runner

![skyrunner][img-skyrunner]

4일 게임잼에서 타 팀이 애니메이션 노드 편집기를 채택. 재빌드 없이 즉시 편집 — 단기간에 복잡한 파쿠르 액션 게임 구현이 가능했다. 위 이미지는 Sky Runner에서 활용된 실제 애니메이션 그래프로, 파쿠르 동작의 복잡한 상태 전이가 노드 연결만으로 표현되어 있다.

---

## References

- Epic Games, **Unreal Engine 5 Source — UEdGraph / UEdGraphNode / UK2Node / UAnimGraphNode**

---

[← Week 10][link-week10] | [Week 12 →][link-week12]

<!-- 이미지 레퍼런스 -->
[img-preview]:    Docs/Images/main-image.png
[img-editor]:     Docs/Images/editor.png
[img-expression]: Docs/Images/expression-graph.png
[img-animation]:  Docs/Images/animation-graph.png
[img-skyrunner]:  Docs/Images/sky-runner.png

<!-- 링크 레퍼런스 -->
[link-week10]: https://github.com/geb0598/DX-Engine/tree/week-10
[link-week12]: https://github.com/geb0598/DX-Engine/tree/week-12
