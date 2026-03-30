import os

def update_mtl_files():
    current_dir = os.getcwd()
    updated_count = 0

    print("MTL 파일 내 TGA -> PNG 텍스트 수정을 시작합니다...")

    for filename in os.listdir(current_dir):
        if filename.lower().endswith(".mtl"):
            mtl_path = os.path.join(current_dir, filename)
            
            try:
                # 파일 읽기 (인코딩 에러 방지를 위해 utf-8, 혹은 기본 설정 사용)
                with open(mtl_path, 'r', encoding='utf-8', errors='ignore') as file:
                    content = file.read()
                
                # .tga 확장자가 포함되어 있는지 확인 (대소문자 무시)
                if '.tga' in content.lower():
                    # .tga 와 .TGA 를 모두 .png 로 치환
                    updated_content = content.replace('.tga', '.png').replace('.TGA', '.png')
                    
                    # 덮어쓰기
                    with open(mtl_path, 'w', encoding='utf-8') as file:
                        file.write(updated_content)
                    
                    print(f"✅ 수정 완료: {filename}")
                    updated_count += 1
                else:
                    print(f"➖ 수정 불필요 (TGA 경로 없음): {filename}")
                    
            except Exception as e:
                print(f"❌ 수정 실패 ({filename}): {e}")

    print(f"\n작업 완료! 총 {updated_count}개의 MTL 파일 경로가 수정되었습니다.")

if __name__ == "__main__":
    update_mtl_files()