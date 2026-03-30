"""
generate_lights.py — Point light grid generator for Junreal-Engine .Scene files

Usage:
    # 라이트만 있는 새 씬 생성
    python generate_lights.py --out Scene/LightTest.scene

    # 기존 씬에 라이트 추가
    python generate_lights.py --base Scene/MySponza.scene --out Scene/SponzaLights.scene

    # 파라미터 커스터마이징
    python generate_lights.py --grid 10 10 10 --spacing 10 20 10 --intensity 3.0 --radius 15.0 --out Scene/LightTest.scene

Options:
    --base      기존 씬 파일 경로 (생략 시 라이트만 있는 씬 생성)
    --out       출력 씬 파일 경로 (필수)
    --grid      X Y Z 그리드 크기 (기본: 10 10 10)
    --origin    X Y Z 그리드 시작 위치 (기본: 0.0 0.0 0.0)
    --spacing   X Y Z 간격 또는 단일 값으로 균등 적용 (기본: 10.0)
    --intensity 라이트 강도 (기본: 3.0)
    --radius    감쇠 반경 (기본: 15.0)
    --falloff   폴오프 지수 (기본: 1.0)
    --color     R G B (0-255, 기본: 255 255 255 흰색)

HARD LIMIT: MAX_POINT_LIGHTS = 1024 (CBufferTypes.h 기준)
"""

import json
import argparse
import sys
from pathlib import Path


MAX_POINT_LIGHTS = 1024


def make_actor(uuid: int, name: str, root_comp_uuid: int) -> dict:
    return {
        "UUID": uuid,
        "Type": "AActor",
        "Name": name,
        "RootComponentUUID": root_comp_uuid,
    }


def make_scene_component(uuid: int, owner_uuid: int, location: list) -> dict:
    return {
        "UUID": uuid,
        "OwnerActorUUID": owner_uuid,
        "ParentComponentUUID": 0,
        "Type": "USceneComponent",
        "RelativeLocation": location,
        "RelativeRotation": [0.0, 0.0, 0.0],
        "RelativeScale": [1.0, 1.0, 1.0],
    }


def make_billboard(uuid: int, owner_uuid: int, parent_uuid: int) -> dict:
    return {
        "UUID": uuid,
        "OwnerActorUUID": owner_uuid,
        "ParentComponentUUID": parent_uuid,
        "Type": "UBillboardComponent",
        "RelativeLocation": [0.0, 0.0, 0.0],
        "RelativeRotation": [0.0, 0.0, 0.0],
        "RelativeScale": [1.0, 1.0, 1.0],
        "TexturePath": "Editor/Icon/PointLight_64x.dds",
    }


def make_point_light(
    uuid: int,
    owner_uuid: int,
    parent_uuid: int,
    intensity: float,
    color: list,
    attenuation_radius: float,
    falloff_exponent: float,
) -> dict:
    return {
        "UUID": uuid,
        "OwnerActorUUID": owner_uuid,
        "ParentComponentUUID": parent_uuid,
        "Type": "UPointLightComponent",
        "RelativeLocation": [0.0, 0.0, 0.0],
        "RelativeRotation": [0.0, 0.0, 0.0],
        "RelativeScale": [1.0, 1.0, 1.0],
        "LightData": {
            "Intensity": intensity,
            "Visible": True,
            "Color": [color[0], color[1], color[2], 255],
        },
        "PointLightData": {
            "AttenuationRadius": attenuation_radius,
            "LightFalloffExponent": falloff_exponent,
        },
    }


def generate_lights(args):
    gx, gy, gz = args.grid
    total = gx * gy * gz

    if total > MAX_POINT_LIGHTS:
        print(
            f"[ERROR] 라이트 수 {total}개 ({gx}×{gy}×{gz})가 한계값 {MAX_POINT_LIGHTS}을 초과합니다.",
            file=sys.stderr,
        )
        print(
            f"        그리드를 줄이거나 --grid 옵션을 조정하세요.",
            file=sys.stderr,
        )
        print(
            f"        예: --grid 10 10 10 (= 1000개, 안전)",
            file=sys.stderr,
        )
        sys.exit(1)

    print(f"[INFO] 그리드: {gx}×{gy}×{gz} = {total}개 포인트 라이트 생성")

    # --- 기존 씬 로드 또는 기본 씬 생성 ---
    if args.base:
        base_path = Path(args.base)
        if not base_path.exists():
            print(f"[ERROR] base 씬 파일을 찾을 수 없습니다: {base_path}", file=sys.stderr)
            sys.exit(1)
        with open(base_path, "r", encoding="utf-8") as f:
            scene = json.load(f)
        print(f"[INFO] 기존 씬 로드: {base_path}")
        start_uuid = scene.get("NextUUID", 1)
    else:
        sp = args.spacing
        _sx = sp[0] if len(sp) == 3 else sp[0]
        _sy = sp[1] if len(sp) == 3 else sp[0]
        _sz = sp[2] if len(sp) == 3 else sp[0]
        scene = {
            "Version": 2,
            "NextUUID": 1,
            "PerspectiveCamera": {
                "FOV": [60.0],
                "FarClip": [4000.0],
                "Location": [args.origin[0] + gx * _sx * 0.5,
                             args.origin[1] - gx * _sy,
                             args.origin[2] + gz * _sz * 0.5],
                "NearClip": [0.1],
                "Rotation": [0.0, 20.0, 0.0],
            },
            "Actors": [],
            "Components": [],
        }
        start_uuid = 1

    actors = scene.setdefault("Actors", [])
    components = scene.setdefault("Components", [])

    uuid = start_uuid
    ox, oy, oz = args.origin
    if len(args.spacing) == 1:
        sx = sy = sz = args.spacing[0]
    else:
        sx, sy, sz = args.spacing

    for iz in range(gz):
        for iy in range(gy):
            for ix in range(gx):
                light_index = iz * gy * gx + iy * gx + ix
                name = f"PointLight_{light_index}"

                actor_uuid      = uuid;      uuid += 1
                root_uuid       = uuid;      uuid += 1
                billboard_uuid  = uuid;      uuid += 1
                light_uuid      = uuid;      uuid += 1

                pos = [
                    ox + ix * sx,
                    oy + iy * sy,
                    oz + iz * sz,
                ]

                actors.append(make_actor(actor_uuid, name, root_uuid))
                components.append(make_scene_component(root_uuid, actor_uuid, pos))
                components.append(make_billboard(billboard_uuid, actor_uuid, root_uuid))
                components.append(
                    make_point_light(
                        light_uuid,
                        actor_uuid,
                        root_uuid,
                        args.intensity,
                        args.color,
                        args.radius,
                        args.falloff,
                    )
                )

    scene["NextUUID"] = uuid

    out_path = Path(args.out)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(scene, f, indent=2, ensure_ascii=False)

    print(f"[INFO] 씬 저장 완료: {out_path}")
    print(f"[INFO] 총 라이트: {total}개 | UUID 범위: {start_uuid} ~ {uuid - 1}")


def main():
    parser = argparse.ArgumentParser(
        description="Junreal-Engine 씬에 포인트 라이트 그리드를 자동 배치합니다."
    )
    parser.add_argument("--base", type=str, default=None,
                        help="기존 씬 파일 경로 (생략 시 새 씬 생성)")
    parser.add_argument("--out", type=str, required=True,
                        help="출력 씬 파일 경로")
    parser.add_argument("--grid", type=int, nargs=3, default=[10, 10, 10],
                        metavar=("X", "Y", "Z"),
                        help="그리드 크기 (기본: 10 10 10, 최대 합계 1024)")
    parser.add_argument("--origin", type=float, nargs=3, default=[0.0, 0.0, 0.0],
                        metavar=("X", "Y", "Z"),
                        help="그리드 시작 좌표 (기본: 0 0 0)")
    parser.add_argument("--spacing", type=float, nargs="+", default=[10.0],
                        metavar="S",
                        help="라이트 간 간격: 단일 값(균등) 또는 X Y Z 3개 (기본: 10.0)")
    parser.add_argument("--intensity", type=float, default=3.0,
                        help="라이트 강도 (기본: 3.0)")
    parser.add_argument("--radius", type=float, default=15.0,
                        help="감쇠 반경 (기본: 15.0)")
    parser.add_argument("--falloff", type=float, default=1.0,
                        help="폴오프 지수 (기본: 1.0)")
    parser.add_argument("--color", type=int, nargs=3, default=[255, 255, 255],
                        metavar=("R", "G", "B"),
                        help="라이트 색상 0-255 (기본: 255 255 255)")

    args = parser.parse_args()

    if len(args.spacing) not in (1, 3):
        parser.error("--spacing은 값 1개(균등) 또는 3개(X Y Z)만 허용합니다.")

    generate_lights(args)


if __name__ == "__main__":
    main()
