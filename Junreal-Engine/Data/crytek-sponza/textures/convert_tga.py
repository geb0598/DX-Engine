import os
from PIL import Image

def convert_tga_to_png():
    # 현재 스크립트가 실행되는 디렉토리 경로
    current_dir = os.getcwd()
    converted_count = 0

    print("TGA -> PNG 변환 작업을 시작합니다...")

    # 디렉토리 내의 모든 파일 순회
    for filename in os.listdir(current_dir):
        if filename.lower().endswith(".tga"):
            tga_path = os.path.join(current_dir, filename)
            # 확장자를 .png로 변경
            png_filename = filename[:-4] + ".png"
            png_path = os.path.join(current_dir, png_filename)
            
            try:
                # 이미지 열고 PNG로 저장
                with Image.open(tga_path) as img:
                    img.save(png_path, "PNG")
                
                # 기존 TGA 파일 삭제 (완전 대체)
                os.remove(tga_path)
                
                print(f"✅ 변환 완료: {filename} -> {png_filename}")
                converted_count += 1
                
            except Exception as e:
                print(f"❌ 변환 실패 ({filename}): {e}")

    print(f"\n작업 완료! 총 {converted_count}개의 파일이 변환되었습니다.")

if __name__ == "__main__":
    convert_tga_to_png()