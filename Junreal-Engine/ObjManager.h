#pragma once
#include "UEContainer.h"
#include "Vector.h"
#include "Enums.h"
#include <regex>

// Raw Data
struct FObjInfo
{
    TArray<FVector> Positions;
    TArray<FVector2D> TexCoords;
    TArray<FVector> Normals;

    TArray<uint32> PositionIndices;
    TArray<uint32> TexCoordIndices;
    TArray<uint32> NormalIndices;

    TArray<FString> MaterialNames; // == 강의 글의 meshMaterials
    TArray<uint32> GroupIndexStartArray; // i번째 Group이 사용하는 indices 범위: GroupIndexStartArray[i] ~ GroupIndexStartArray[i+1]
    TArray<uint32> GroupMaterialArray; // i번쩨 Group이 사용하는 MaterialInfos 인덱스 넘버

    FString ObjFileName;

    bool bHasMtl = true;
};



struct FObjImporter
{
    // TODO: 변수이름 가독성 있게 재설정
public:
    static bool LoadObjModel(const FString& InFileName, FObjInfo* const OutObjInfo, TArray<FObjMaterialInfo>& OutMaterialInfos, bool bIsRHCoordSys, bool bComputeNormals)
    {
        // mtl 파싱할 때 필요한 정보들. 이거를 함수 밖으로 보내줘야 할수도? obj 파싱하면서 저장.(아래 링크 기반) 나중에 형식 바뀔수도 있음
        // https://www.braynzarsoft.net/viewtutorial/q16390-22-loading-static-3d-models-obj-format
        uint32 subsetCount = 0;

        FString MtlFileName;

        // Make sure we have a default if no tex coords or normals are defined
        bool bHasTexcoord = false;
        bool bHasNormal = false;

        FString MaterialNameTemp;
        // uint32 VertexPositionIndexTemp;
        // uint32 VertexTexIndexTemp;
        // uint32 VertexNormalIndexTemp;

        FString Face;
        uint32 VIndex = 0; // 현재 파싱중인 vertex의 넘버(start: 0. 중복 고려x)
        uint32 MeshTriangles = 0; // 현재까지 파싱된 Triangle 개수

        size_t pos = InFileName.find_last_of("/\\");
        std::string ObjDir = (pos == std::string::npos) ? "" : InFileName.substr(0, pos + 1);

        std::ifstream FileIn(InFileName.c_str());

        if (!FileIn)
        {
            UE_LOG("The filename %s does not exist!", InFileName.c_str());
            return false;
        }

        // obj 파싱 시작
        OutObjInfo->ObjFileName = FString(InFileName.begin(), InFileName.end()); // 아스키 코드라고 가정

        FString Line;
        while (std::getline(FileIn, Line))
        {
            if (Line.empty()) continue;

            Line.erase(0, Line.find_first_not_of(" \t\n\r"));

            // 주석(#) 처리
            if (Line[0] == '#')   // wide literal
                continue;

            if (Line.rfind("v ", 0) == 0) // 정점 좌표 (v x y z)
            {
                std::stringstream wss(Line.substr(2));
                float vx, vy, vz;
                wss >> vx >> vy >> vz;

                if (bIsRHCoordSys)
                {
                    //OutObjInfo->Positions.push_back(FVector(vx, -vy, vz));
                    OutObjInfo->Positions.push_back(FVector(vz, -vy, vx));
                }
                else
                    OutObjInfo->Positions.push_back(FVector(vx, vy, vz));
            }
            else if (Line.rfind("vt ", 0) == 0) // 텍스처 좌표 (vt u v)
            {
                std::stringstream wss(Line.substr(3));
                float u, v;
                wss >> u >> v;

                if (bIsRHCoordSys)
                    OutObjInfo->TexCoords.push_back(FVector2D(u, 1.0f - v));
                else
                    OutObjInfo->TexCoords.push_back(FVector2D(u, v));

                bHasTexcoord = true;
            }
            else if (Line.rfind("vn ", 0) == 0) // 법선 (vn x y z)
            {
                std::stringstream wss(Line.substr(3));
                float nx, ny, nz;
                wss >> nx >> ny >> nz;

                if (bIsRHCoordSys)
                    OutObjInfo->Normals.push_back(FVector(nz, -ny, nx));
                    //OutObjInfo->Normals.push_back(FVector(nx, -ny, nz));
                else
                    //OutObjInfo->Normals.push_back(FVector(nz, ny, nx));
                    OutObjInfo->Normals.push_back(FVector(nx, ny, nz));

                bHasNormal = true;
            }
            else if (Line.rfind("g ", 0) == 0) // 그룹 (g groupName)
            {
                /*GroupIndexStartArray.push_back(VIndex);
                subsetCount++;*/
            }
            else if (Line.rfind("f ", 0) == 0) // 면 (f v1/vt1/vn1 v2/vt2/vn2 ...)
            {
                Face = Line.substr(2); // ex: "3/2/2 3/3/2 3/4/2 "

                if (Face.length() <= 0)
                {
                    continue;
                }

                std::stringstream wss(Face);
                FString VertexDef; // ex: 3/2/2

                // 2) 실제 Face 파싱
                TArray<FFaceVertex> LineFaceVertices;
                while (wss >> VertexDef)
                {
                    FFaceVertex FaceVertex = ParseVertexDef(VertexDef);
                    LineFaceVertices.push_back(FaceVertex);
                }

                //3) FaceVertices에 그대로 파싱된 걸로, 다시 트라이앵글에 맞춰 인덱스 배열들에 넣기
                for (uint32 i = 0; i < 3; ++i)
                {
                    OutObjInfo->PositionIndices.push_back(LineFaceVertices[i].PositionIndex);
                    OutObjInfo->TexCoordIndices.push_back(LineFaceVertices[i].TexCoordIndex);
                    OutObjInfo->NormalIndices.push_back(LineFaceVertices[i].NormalIndex);
                    ++VIndex;
                }
                ++MeshTriangles;

                for (uint32 i = 3; i < LineFaceVertices.size(); ++i)
                {
                    OutObjInfo->PositionIndices.push_back(LineFaceVertices[0].PositionIndex);
                    OutObjInfo->TexCoordIndices.push_back(LineFaceVertices[0].TexCoordIndex);
                    OutObjInfo->NormalIndices.push_back(LineFaceVertices[0].NormalIndex);
                    ++VIndex;

                    OutObjInfo->PositionIndices.push_back(LineFaceVertices[i-1].PositionIndex);
                    OutObjInfo->TexCoordIndices.push_back(LineFaceVertices[i-1].TexCoordIndex);
                    OutObjInfo->NormalIndices.push_back(LineFaceVertices[i-1].NormalIndex);
                    ++VIndex;

                    OutObjInfo->PositionIndices.push_back(LineFaceVertices[i].PositionIndex);
                    OutObjInfo->TexCoordIndices.push_back(LineFaceVertices[i].TexCoordIndex);
                    OutObjInfo->NormalIndices.push_back(LineFaceVertices[i].NormalIndex);
                    ++VIndex;

                    ++MeshTriangles;
                }
            }
            else if (Line.rfind("mtllib ", 0) == 0)
            {
                MtlFileName = ObjDir + Line.substr(7);
            }
            else if (Line.rfind("usemtl ", 0) == 0)
            {
                MaterialNameTemp = Line.substr(7);
                OutObjInfo->MaterialNames.push_back(MaterialNameTemp);

                // material 하나 당 group 하나라고 가정. 현재 단계에서는 usemtl로 group을 분리하는 게 편함.
                OutObjInfo->GroupIndexStartArray.push_back(VIndex);
                subsetCount++;
            }
            else
            {
                UE_LOG("While parsing the filename %s, the following unknown symbol was encountered: \'%s\'", InFileName.c_str(), Line.c_str());
            }
        }

        
        // GroupIndexStartArray 마무리 작업
        if (subsetCount == 0) //Check to make sure there is at least one subset
        {
            OutObjInfo->GroupIndexStartArray.push_back(0);		//Start index for this subset
            subsetCount++;
        }
        OutObjInfo->GroupIndexStartArray.push_back(VIndex); //There won't be another index start after our last subset, so set it here

        //sometimes "g" is defined at the very top of the file, then again before the first group of faces.
        //This makes sure the first subset does not conatain "0" indices.
        if (OutObjInfo->GroupIndexStartArray[1] == 0)
        {
            OutObjInfo->GroupIndexStartArray.erase(OutObjInfo->GroupIndexStartArray.begin() + 1);
            subsetCount--;
        }

        // default 값 설정
        if (!bHasNormal)
        {
            OutObjInfo->Normals.push_back(FVector(0.0f, 0.0f, 0.0f));
        }
        if (!bHasTexcoord)
        {
            OutObjInfo->TexCoords.push_back(FVector2D(0.0f, 0.0f));
        }

        FileIn.close();

        // TODO: Normal 다시 계산

        // Material 파싱 시작
        FileIn.open(MtlFileName.c_str());

        if (MtlFileName.empty())
        {
            OutObjInfo->bHasMtl = false;
            return true;
        }

        if (!FileIn)
        {
            UE_LOG("The filename %s does not exist!(obj filename: %s)", MtlFileName.c_str(), InFileName.c_str());
            return false;
        }

        OutMaterialInfos.reserve(OutObjInfo->MaterialNames.size());
        /*OutMaterialInfos->resize(OutObjInfo->MaterialNames.size());*/
        uint32 MatCount = static_cast<uint32>(OutMaterialInfos.size());
        //FString line;
        while (std::getline(FileIn, Line))
        {
            if (Line.empty()) continue;

            Line.erase(0, Line.find_first_not_of(" \t\n\r"));
            // 주석(#) 처리
            if (Line[0] == '#')   // wide literal
                continue;

            if (Line.rfind("Kd ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float vx, vy, vz;
                wss >> vx >> vy >> vz;

                OutMaterialInfos[MatCount - 1].DiffuseColor = FVector(vx, vy, vz);
            }
            else if (Line.rfind("Ka ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float vx, vy, vz;
                wss >> vx >> vy >> vz;

                OutMaterialInfos[MatCount - 1].AmbientColor = FVector(vx, vy, vz);
            }
            else if (Line.rfind("Ke ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float vx, vy, vz;
                wss >> vx >> vy >> vz;

                OutMaterialInfos[MatCount - 1].EmissiveColor = FVector(vx, vy, vz);
            }
            else if (Line.rfind("Ks ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float vx, vy, vz;
                wss >> vx >> vy >> vz;

                OutMaterialInfos[MatCount - 1].SpecularColor = FVector(vx, vy, vz);
            }
            else if (Line.rfind("Tf ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float vx, vy, vz;
                wss >> vx >> vy >> vz;

                OutMaterialInfos[MatCount - 1].TransmissionFilter = FVector(vx, vy, vz);
            }
            else if (Line.rfind("Tr ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float value;
                wss >> value;

                OutMaterialInfos[MatCount - 1].Transparency = value;
            }
            else if (Line.rfind("d ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float value;
                wss >> value;

                OutMaterialInfos[MatCount - 1].Transparency = 1.0f - value;
            }
            else if (Line.rfind("Ni ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float value;
                wss >> value;

                OutMaterialInfos[MatCount - 1].OpticalDensity = value;
            }
            else if (Line.rfind("Ns ", 0) == 0)
            {
                std::stringstream wss(Line.substr(3));
                float value;
                wss >> value;

                OutMaterialInfos[MatCount - 1].SpecularExponent = value;
            }
            else if (Line.rfind("illum ", 0) == 0)
            {
                std::stringstream wss(Line.substr(6));
                float value;
                wss >> value;

                OutMaterialInfos[MatCount - 1].IlluminationModel = static_cast<int32>(value);
            }
            else if (Line.rfind("map_Kd ", 0) == 0)
            {
                FString TextureFileName;
                if (Line.substr(7).rfind(ObjDir) != 0)
                {
                    TextureFileName = ObjDir + Line.substr(7);
                }
                else
                {
                    TextureFileName = Line.substr(7);
                }
                std::replace(TextureFileName.begin(), TextureFileName.end(), '\\', '/');
                OutMaterialInfos[MatCount - 1].DiffuseTextureFileName = FName(TextureFileName);
            }
            else if (Line.rfind("map_d ", 0) == 0)
            {
                FString TextureFileName;
                if (Line.substr(7).rfind(ObjDir) != 0)
                {
                    TextureFileName = ObjDir + Line.substr(7);
                }
                else
                {
                    TextureFileName = Line.substr(7);
                }
                OutMaterialInfos[MatCount - 1].TransparencyTextureFileName = TextureFileName;
            }
            else if (Line.rfind("map_Ka ", 0) == 0)
            {
                FString TextureFileName;
                if (Line.substr(7).rfind(ObjDir) != 0)
                {
                    TextureFileName = ObjDir + Line.substr(7);
                }
                else
                {
                    TextureFileName = Line.substr(7);
                }
                OutMaterialInfos[MatCount - 1].AmbientTextureFileName = TextureFileName;
            }
            else if (Line.rfind("map_Ks ", 0) == 0)
            {
                FString TextureFileName;
                if (Line.substr(7).rfind(ObjDir) != 0)
                {
                    TextureFileName = ObjDir + Line.substr(7);
                }
                else
                {
                    TextureFileName = Line.substr(7);
                }
                OutMaterialInfos[MatCount - 1].SpecularTextureFileName = TextureFileName;
            }
            else if (Line.rfind("map_Ns ", 0) == 0)
            {
                FString TextureFileName;
                if (Line.substr(7).rfind(ObjDir) != 0)
                {
                    TextureFileName = ObjDir + Line.substr(7);
                }
                else
                {
                    TextureFileName = Line.substr(7);
                }
                OutMaterialInfos[MatCount - 1].SpecularExponentTextureFileName = TextureFileName;
            }
            else if (Line.rfind("map_Ke ", 0) == 0)
            {
                FString TextureFileName;
                if (Line.substr(7).rfind(ObjDir) != 0)
                {
                    TextureFileName = ObjDir + Line.substr(7);
                }
                else
                {
                    TextureFileName = Line.substr(7);
                }
                OutMaterialInfos[MatCount - 1].EmissiveTextureFileName = TextureFileName;
            }
            else if (Line.rfind("map_Bump ", 0) == 0)
            {
                //FString TextureFileName;
                //if (line.substr(7).rfind(objDir) != 0)
                //{
                //    TextureFileName = objDir + line.substr(7);
                //}
                //else
                //{
                //    TextureFileName = line.substr(7);
                //}
                //std::replace(TextureFileName.begin(), TextureFileName.end(), '\\', '/');
                //OutMaterialInfos[MatCount - 1].NormalTextureName = TextureFileName;

                FString Rest = Line.substr(9);

                std::regex Re(R"rgx("([^"]+\.png)"|([^\s"]+\.png))rgx", std::regex::icase);
                std::sregex_iterator it(Rest.begin(), Rest.end(), Re), End;

                if (it != End)
                {
                    // 마지막 매치를 채택
                    std::smatch M = *it; ++it;
                    for (; it != End; ++it) M = *it;

                    FString TextureFileName = M[1].matched ? M[1].str() : M[2].str();

                    // 경로 정규화 및 상대경로면 objDir 접두
                    if (TextureFileName.rfind(ObjDir, 0) != 0)
                    {
                        TextureFileName = ObjDir + TextureFileName;
                    }

                    std::replace(TextureFileName.begin(), TextureFileName.end(), '\\', '/');

                    OutMaterialInfos[MatCount - 1].NormalTextureName = FName(TextureFileName);
                }
            }
            else if (Line.rfind("newmtl ", 0) == 0)
            {
                FObjMaterialInfo TempMatInfo;
                TempMatInfo.MaterialName = Line.substr(7);

                OutMaterialInfos.push_back(TempMatInfo);
                ++MatCount;
            }
            else
            {
                //UE_LOG("While parsing the filename %s, the following unknown symbol was encountered: %s", InFileName.c_str(), line.c_str());
            }
        }

        FileIn.close();

        // 각 Group이 가지는 Material의 인덱스 값 설정
        for (uint32 i = 0; i < OutObjInfo->MaterialNames.size(); ++i)
        {
            bool HasMat = false;
            for (uint32 j = 0; j < OutMaterialInfos.size(); ++j)
            {
                if (OutObjInfo->MaterialNames[i] == OutMaterialInfos[j].MaterialName)
                {
                    OutObjInfo->GroupMaterialArray.push_back(j);
                    HasMat = true;
                }
            }

            // usemtl 다음 문자열이 mtl 파일 안에 없으면, 첫번째 material로 설정
            if (!HasMat)
            {
                OutObjInfo->GroupMaterialArray.push_back(0); 
            }
        }

		return true;
    }

    struct VertexKey
    {
        uint32 PosIndex;
        uint32 TexIndex;
        uint32 NormalIndex;

        bool operator==(const VertexKey& Other) const
        {
            return PosIndex == Other.PosIndex &&
                TexIndex == Other.TexIndex &&
                NormalIndex == Other.NormalIndex;
        }
    };

    struct VertexKeyHash
    {
        size_t operator()(const VertexKey& Key) const
        {
            // 간단한 해시 조합
            return ((size_t)Key.PosIndex * 73856093) ^
                ((size_t)Key.TexIndex * 19349663) ^
                ((size_t)Key.NormalIndex * 83492791);
        }
    };

    static void ConvertToStaticMesh(const FObjInfo& InObjInfo, const TArray<FObjMaterialInfo>& InMaterialInfos, FStaticMesh* const OutStaticMesh)
    {
        OutStaticMesh->PathFileName = InObjInfo.ObjFileName;
        uint32 NumDuplicatedVertex = static_cast<uint32>(InObjInfo.PositionIndices.size());

        // 1) Vertices, Indices 설정: 해시로 빠르게 중복찾기
        std::unordered_map<VertexKey, uint32, VertexKeyHash> VertexMap;
        for (uint32 CurIndex = 0; CurIndex < NumDuplicatedVertex; ++CurIndex)
        {
            VertexKey Key{ InObjInfo.PositionIndices[CurIndex],
                           InObjInfo.TexCoordIndices[CurIndex],
                           InObjInfo.NormalIndices[CurIndex] };

            auto It = VertexMap.find(Key);
            if (It != VertexMap.end())
            {
                // 이미 존재하는 정점
                OutStaticMesh->Indices.push_back(It->second);
            }
            else
            {
                // 새 정점 추가
                FVector Pos = InObjInfo.Positions[Key.PosIndex];
                FVector Normal = InObjInfo.Normals[Key.NormalIndex];
                FVector2D TexCoord = InObjInfo.TexCoords[Key.TexIndex];
                FVector4 Color(1, 1, 1, 1);

                FNormalVertex NormalVertex(Pos, Normal, Color, TexCoord);
                OutStaticMesh->Vertices.push_back(NormalVertex);

                uint32 NewIndex = static_cast<uint32>(OutStaticMesh->Vertices.size() - 1);
                OutStaticMesh->Indices.push_back(NewIndex);

                VertexMap[Key] = NewIndex;
            }
        }
        
        // 2) Material 관련 각 case 처리
        if (!InObjInfo.bHasMtl)
        {
            OutStaticMesh->bHasMaterial = false;
            return;
        }

        OutStaticMesh->bHasMaterial = true;
        uint32 NumGroup = static_cast<uint32>(InObjInfo.MaterialNames.size());
        OutStaticMesh->GroupInfos.resize(NumGroup);
        if (InMaterialInfos.size() == 0)
        {
            UE_LOG("\'%s\''s InMaterialInfos's size is 0!");
            return;
        }

        // 3) 리소스 매니저에 Material 리소스 맵핑
        /*for (const FObjMaterialInfo& InMaterialInfo : InMaterialInfos)
        {
            UMaterial* Material = NewObject<UMaterial>();
            Material->SetMaterialInfo(InMaterialInfo);

            UResourceManager::GetInstance().Add<UMaterial>(InMaterialInfo.MaterialName, Material);
        }*/

        // 4) GroupInfo 정보 설정
        for (uint32 i = 0; i < NumGroup; ++i)
        {
            OutStaticMesh->GroupInfos[i].StartIndex = InObjInfo.GroupIndexStartArray[i];
            OutStaticMesh->GroupInfos[i].IndexCount = InObjInfo.GroupIndexStartArray[i + 1] - InObjInfo.GroupIndexStartArray[i];

            // <생각의 흔적...>
            // MaterialInfo를 그대로 가져오는 게 아니라, 해당 InMaterialInfos[InObjInfo.GroupMaterialArray[i]].MaterialName으로 가져오기
            // 그리고 StaticMeshComp쪽에서, 이 초기 Info name들을 이용해, 자기가 갖고있는 names FString배열에 집어넣는 거야.
            // 그러면, ResourdeManger를 가져와서, Resoures map 배열에 집어넣는거야. 관련 설정도 필요하겠지.
            // UMaterial을 써야 하나?. 아니면, 차라리. 이거 그대로 넣고. 나중에 StaticMeshComp의 SetStaticMesh(fileName)에서 해줄까?
            // 
            // 최종 로직:
            // 1) for문 밖에서: InMaterialInfos의 각 요소마다, Umaterial 생성해서, 거기의 생성자에서 InMaterialInfo들을 설정해주는 거야.
            // 그렇게 생성된 Umaterial과, InMaterialInfos.MaterialName을 맵핑해서, 리소스 매니저의 Add 함수로 집어넣는 거지.
            // 2) 여기서: OutStaticMesh는 MaterialInfo 대신 파일네임만 가지게 하고.(변수이름 변경)
            // 3) 이후: StaticMeshComp의 SetStaticMesh(filename) 내부에서, OutStaticMesh->GroupInfos[i].InitialMatNames와, dirty flag를 가지고, 멤버변수 MatSlots를 설정해줘.
            // 일단 여기까지 하고, 나중에, imgui에서 material slot의 matName을 바꾸면, dirty flag true로 바꾸는 로직도 설정하기.->완료.
            //OutStaticMesh->GroupInfos[i].MaterialInfo = InMaterialInfos[InObjInfo.GroupMaterialArray[i]];
            OutStaticMesh->GroupInfos[i].InitialMaterialName = InMaterialInfos[InObjInfo.GroupMaterialArray[i]].MaterialName;
        }

        const size_t vertexCount = OutStaticMesh->Vertices.size();
        std::vector<FVector> AccumT(vertexCount, FVector(0, 0, 0));
        std::vector<FVector> AccumB(vertexCount, FVector(0, 0, 0));

        for (size_t i = 0; i < OutStaticMesh->Indices.size(); i += 3)
        {
            uint32 I0 = OutStaticMesh->Indices[i + 0];
            uint32 I1 = OutStaticMesh->Indices[i + 1];
            uint32 I2 = OutStaticMesh->Indices[i + 2];

            const auto& V0 = OutStaticMesh->Vertices[I0];
            const auto& V1 = OutStaticMesh->Vertices[I1];
            const auto& V2 = OutStaticMesh->Vertices[I2];

            const FVector  P0 = V0.pos, P1 = V1.pos, P2 = V2.pos;
            const FVector2D UV0 = V0.tex, UV1 = V1.tex, UV2 = V2.tex;

            FVector E1 = P1 - P0;
            FVector E2 = P2 - P0;
            FVector2D D1 = UV1 - UV0;
            FVector2D D2 = UV2 - UV0;

            float Det = D1.X * D2.Y - D1.Y * D2.X;
            if (abs(Det) < 1e-20f) {
                // UV 퇴화: N과 직교하는 임의 T/B 생성(fallback)
                FVector N = (V0.normal + V1.normal + V2.normal).GetSafeNormal();
                FVector A = (abs(N.Z) < 0.999f) ? FVector(0, 0, 1) : FVector(0, 1, 0);
                FVector T = (FVector::Cross(A, N)).GetSafeNormal();
                FVector B = FVector::Cross(N, T);
                AccumT[I0] += T; AccumT[I1] += T; AccumT[I2] += T;
                AccumB[I0] += B; AccumB[I1] += B; AccumB[I2] += B;
                continue;
            }

            float R = 1.0f / Det;
            FVector Tface = (E1 * D2.Y - E2 * D1.Y) * R; // U 축
            FVector Bface = (E2 * D1.X - E1 * D2.X) * R; // V 축

            // 면적 가중치(권장): |cross(e1,e2)| 반
            float W = FVector::Cross(E1, E2).Size();
            Tface *= W; Bface *= W;

            AccumT[I0] += Tface; AccumT[I1] += Tface; AccumT[I2] += Tface;
            AccumB[I0] += Bface; AccumB[I1] += Bface; AccumB[I2] += Bface;
        }

        for (size_t v = 0; v < vertexCount; ++v)
        {
            FVector N = OutStaticMesh->Vertices[v].normal.GetSafeNormal();
            FVector T = AccumT[v];

            T = (T - N * FVector::Dot(N, T)).GetSafeNormal();
            if (!(T.Size() < 1e-5))
            {
                FVector BSum = AccumB[v];
                float H = (FVector::Dot(FVector::Cross(N, T), BSum) < 0.0f) ? -1.0f : +1.0f;
                OutStaticMesh->Vertices[v].tangent = FVector4(T.X, T.Y, T.Z, H);
            }
            else
            {
                FVector A = (abs(N.Z) < 0.999f) ? FVector(0, 0, 1) : FVector(0, 1, 0);
                FVector Tf = FVector::Cross(A, N).GetSafeNormal();
                OutStaticMesh->Vertices[v].tangent = FVector4(Tf.X, Tf.Y, Tf.Z, -1.0f);
            }
        }
    }

private:
    struct FFaceVertex
    {
        uint32 PositionIndex;
        uint32 TexCoordIndex;
        uint32 NormalIndex;
    };

    //없는 건 0으로 넣음
    static FFaceVertex ParseVertexDef(const FString& InVertexDef)
    {
        FString vertPart;
        uint32 whichPart = 0;

        uint32 VertexPositionIndexTemp;
        uint32 VertexTexIndexTemp;
        uint32 VertexNormalIndexTemp;
        for (int j = 0; j < InVertexDef.length(); ++j)
        {
            if (InVertexDef[j] != '/')	//If there is no divider "/", add a char to our vertPart
                vertPart += InVertexDef[j];

            //If the current char is a divider "/", or its the last character in the string
            if (InVertexDef[j] == '/' || j == InVertexDef.length() - 1)
            {
                std::istringstream stringToInt(vertPart);	//Used to convert wstring to int

                if (whichPart == 0)	//If vPos
                {
                    stringToInt >> VertexPositionIndexTemp;
                    VertexPositionIndexTemp -= 1;		//subtract one since c++ arrays start with 0, and obj start with 1

                    //Check to see if the vert pos was the only thing specified
                    if (j == InVertexDef.length() - 1)
                    {
                        VertexNormalIndexTemp = 0;
                        VertexTexIndexTemp = 0;
                    }
                }

                else if (whichPart == 1)	//If vTexCoord
                {
                    if (vertPart != "")	//Check to see if there even is a tex coord
                    {
                        stringToInt >> VertexTexIndexTemp;
                        VertexTexIndexTemp -= 1;	//subtract one since c++ arrays start with 0, and obj start with 1
                    }
                    else	//If there is no tex coord, make a default
                        VertexTexIndexTemp = 0;

                    //If the cur. char is the second to last in the string, then
                    //there must be no normal, so set a default normal
                    if (j == InVertexDef.length() - 1)
                        VertexNormalIndexTemp = 0;

                }
                else if (whichPart == 2)	//If vNorm
                {
                    stringToInt >> VertexNormalIndexTemp;
                    VertexNormalIndexTemp -= 1;		//subtract one since c++ arrays start with 0, and obj start with 1
                }

                vertPart = "";	//Get ready for next vertex part
                whichPart++;	//Move on to next vertex part					
            }
        }

        FFaceVertex Result = FFaceVertex(VertexPositionIndexTemp, VertexTexIndexTemp, VertexNormalIndexTemp);
        return Result;
    }

};

class UStaticMesh;

class FObjManager
{
private:
    static TMap<FString, FStaticMesh*> ObjStaticMeshMap;

public:
	static void Preload();
	static void Clear();

    static FStaticMesh* LoadObjStaticMeshAsset(const FString& PathFileName);
    static UStaticMesh* LoadObjStaticMesh(const FString& PathFileName);
};