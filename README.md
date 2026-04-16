# DX-Engine

> C++17 · DirectX 11 기반 게임 엔진 — 12주간 렌더링·물리·애니메이션 시스템을 밑바닥부터 구현

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![DirectX 11](https://img.shields.io/badge/DirectX-11-0078D4?logo=microsoft&logoColor=white)
![PhysX](https://img.shields.io/badge/PhysX-4.1.2-76B900?logo=nvidia&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?logo=windows&logoColor=white)
![Visual Studio 2022](https://img.shields.io/badge/Visual_Studio-2022-5C2D91?logo=visualstudio&logoColor=white)

---

## Overview

크래프톤 정글 부트캠프에서 12주간 제작한 DirectX 11 게임 엔진. 매 주차마다 렌더링 파이프라인, 물리 엔진, 애니메이션 시스템 등 하나의 서브시스템을 새로 설계·구현하고 브랜치로 스냅샷을 남겼다. UE4/UE5 아키텍처를 레퍼런스로 삼되 외부 게임 엔진 없이 C++ · HLSL · PhysX SDK만으로 구현했다.

---

## Development Log

| 브랜치 | 주제 | 핵심 구현 |
|--------|------|-----------|
| [week-05][w05] | 엔진 인프라 | Hi-Z GPU Occlusion Culling, OBJ 임포터, FArchive 직렬화, 커스텀 ThreadPool |
| [week-06][w06] | 충돌 · 라이팅 | SAT 기반 OBB 충돌, Point Light 디퍼드 라이팅, FXAA |
| [week-07][w07] | Forward+ | **Compute Shader 2.5D 타일 라이트 컬링** (AMD 2012 논문 기반), 셰이더 핫 리로드 |
| [week-08][w08] | 그림자 | PCF · VSM · SAVSM Shadow Map, Shadow Atlas |
| [week-09][w09] | 게임잼 · 엔진 시스템 | Delegate, FWeakObjectPtr, Actor Pool, Post-Processing, Lua 스크립팅 |
| [week-10][w10] | 스켈레탈 메시 | CPU 스키닝, 계층적 본 변환, 노말 스키닝 |
| [week-11][w11] | 애니메이션 에디터 | Blueprint 노드 에디터, Expression Tree 평가 엔진, AnimGraph 컴파일러 |
| [week-12][w12] | 파티클 시스템 | Cascade 스타일 에셋/런타임 이중 계층, 연속 메모리 풀, Base+Payload 확장 구조 |
| [week-final][wf] | PhysX 통합 | PhysX 4.1.2 통합, PxVehicle4W 차량 시스템, 게임잼 Dumb Rider |

---

## Tech Stack

| 구분 | 항목 |
|------|------|
| Language | C++17, HLSL (Shader Model 5.0) |
| Graphics API | Direct3D 11 |
| Physics | NVIDIA PhysX 4.1.2 |
| Animation | FBX SDK |
| UI | Dear ImGui |
| Scripting | Lua |
| Build | Visual Studio 2022, MSBuild |

---

[w05]: https://github.com/geb0598/DX-Engine/tree/week-05
[w06]: https://github.com/geb0598/DX-Engine/tree/week-06
[w07]: https://github.com/geb0598/DX-Engine/tree/week-07
[w08]: https://github.com/geb0598/DX-Engine/tree/week-08
[w09]: https://github.com/geb0598/DX-Engine/tree/week-09
[w10]: https://github.com/geb0598/DX-Engine/tree/week-10
[w11]: https://github.com/geb0598/DX-Engine/tree/week-11
[w12]: https://github.com/geb0598/DX-Engine/tree/week-12
[wf]:  https://github.com/geb0598/DX-Engine/tree/week-final
