#include "pch.h"

#include <wrl.h>

#include "Shader.h"

UShader::~UShader()
{
    ReleaseResources();
}

// 두 개의 셰이더 파일을 받는 주요 Load 함수
void UShader::Load(const FString& InCommandString, ID3D11Device* InDevice)
{
    assert(InDevice);
    std::stringstream Tokenizer(InCommandString);
    FString Token;

    FString ShaderFilePath;
    FString VSEntryPoint = "mainVS"; // 기본값
    FString PSEntryPoint = "mainPS"; // 기본값
    
    TArray<FString> MacroStrings;

    Tokenizer >> ShaderFilePath;

    while (Tokenizer >> Token)
    {
        if (Token == "-D")
        {
            Tokenizer >> Token; // 매크로 정의 문자열 (e.g., "USE_TEXTURE=1")
            MacroStrings.push_back(Token);
        }
        else if (Token == "-VS")
        {
            Tokenizer >> VSEntryPoint;
        }
        else if (Token == "-PS")
        {
            Tokenizer >> PSEntryPoint;
        }
    }

    TArray<FString> MacroNames;
    TArray<FString> MacroDefinitions;
    TArray<D3D_SHADER_MACRO> Macros;

    MacroNames.reserve(MacroStrings.size());
    MacroDefinitions.reserve(MacroStrings.size());

    for (const auto& MacroString : MacroStrings)
    {
        size_t EqualPos = MacroString.find('=');
        if (EqualPos != std::string::npos)
        {
            MacroNames.push_back(MacroString.substr(0, EqualPos));
            MacroDefinitions.push_back(MacroString.substr(EqualPos + 1, std::string::npos));
        }
        else
        {
            MacroNames.push_back(MacroString);
            MacroDefinitions.push_back("1");
        }
        Macros.push_back({MacroNames.back().c_str(), MacroDefinitions.back().c_str()});
    }
    Macros.push_back({nullptr, nullptr});

    std::wstring wFilePath(ShaderFilePath.begin(), ShaderFilePath.end());

    std::wstring WFilePath;
    WFilePath = std::wstring(InCommandString.begin(), InCommandString.end());

    HRESULT hResult;
    UINT Flag = 0;
    ID3DBlob* ErrorBlob = nullptr;
#ifdef _DEBUG
    Flag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    if (!VSEntryPoint.empty())
    {
        hResult = D3DCompileFromFile(wFilePath.c_str(), Macros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE,
            VSEntryPoint.c_str(), "vs_5_0", Flag, 0, &VSBlob, &ErrorBlob);

        if (FAILED(hResult))
        {
            if (ErrorBlob)
            {
                char* msg = (char*)ErrorBlob->GetBufferPointer();
                UE_LOG("Shader '%s' (VS) compile error: %s", ShaderFilePath.c_str(), msg);
                ErrorBlob->Release();
            }
            return;
        }

        hResult = InDevice->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), nullptr, &VertexShader);
    }

    if (!PSEntryPoint.empty())
    {
        if (ErrorBlob) { ErrorBlob->Release(); ErrorBlob = nullptr; }

        hResult = D3DCompileFromFile(wFilePath.c_str(), Macros.data(), D3D_COMPILE_STANDARD_FILE_INCLUDE,
            PSEntryPoint.c_str(), "ps_5_0", Flag, 0, &PSBlob, &ErrorBlob);
        
        if (FAILED(hResult))
        {
            if (ErrorBlob)
            {
                char* msg = (char*)ErrorBlob->GetBufferPointer();
                UE_LOG("Shader '%s' (PS) compile error: %s", ShaderFilePath.c_str(), msg);
                ErrorBlob->Release();
            }
            return;
        }

        hResult = InDevice->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), nullptr, &PixelShader);
    }
    
    CreateInputLayout(InDevice, ShaderFilePath);
}

void UShader::CreateInputLayout(ID3D11Device* Device, const FString& InShaderPath)
{
    TArray<D3D11_INPUT_ELEMENT_DESC> descArray = UResourceManager::GetInstance().GetProperInputLayout(InShaderPath);
    if (descArray.Num() == 0)
    {
        return;
    }
    const D3D11_INPUT_ELEMENT_DESC* layout = descArray.data();
    uint32 layoutCount = static_cast<uint32>(descArray.size());

    HRESULT hr = Device->CreateInputLayout(
        layout,
        layoutCount,
        VSBlob->GetBufferPointer(), 
        VSBlob->GetBufferSize(),
        &InputLayout);
    assert(SUCCEEDED(hr));
}

void UShader::ReleaseResources()
{
    if (VSBlob)
    {
        VSBlob->Release();
        VSBlob = nullptr;
    }
    if (PSBlob)
    {
        PSBlob->Release();
        PSBlob = nullptr;
    }
    if (InputLayout)
    {
        InputLayout->Release();
        InputLayout = nullptr;
    }
    if (VertexShader)
    {
        VertexShader->Release();
        VertexShader = nullptr;
    }
    if (PixelShader)
    {
        PixelShader->Release();
        PixelShader = nullptr;
    }
}
