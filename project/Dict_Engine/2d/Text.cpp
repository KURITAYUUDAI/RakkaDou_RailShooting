#define NOMINMAX
#include "Text.h"
#include "PSOManager.h"
#include "ImGuiManager.h"

#include <algorithm>
#include <array>
#include <vector>

namespace
{
    int HexValue(char32_t c)
    {
        if (c >= U'0' && c <= U'9')
        {
            return static_cast<int>(c - U'0');
        }
        if (c >= U'a' && c <= U'f')
        {
            return static_cast<int>(c - U'a') + 10;
        }
        if (c >= U'A' && c <= U'F')
        {
            return static_cast<int>(c - U'A') + 10;
        }

        return -1;
    }

    bool TryParseHexColor(const std::u32string& value, Vector4& outColor)
    {
        if (value.size() != 7 && value.size() != 9)
        {
            return false;
        }
        if (value[0] != U'#')
        {
            return false;
        }

        auto parseByte = [&](size_t index, uint32_t& outValue) -> bool
        {
            const int high = HexValue(value[index]);
            const int low = HexValue(value[index + 1]);
            if (high < 0 || low < 0)
            {
                return false;
            }

            outValue = static_cast<uint32_t>((high << 4) | low);
            return true;
        };

        uint32_t r = 0;
        uint32_t g = 0;
        uint32_t b = 0;
        uint32_t a = 255;
        if (!parseByte(1, r) || !parseByte(3, g) || !parseByte(5, b))
        {
            return false;
        }
        if (value.size() == 9 && !parseByte(7, a))
        {
            return false;
        }

        outColor = {
            static_cast<float>(r) / 255.0f,
            static_cast<float>(g) / 255.0f,
            static_cast<float>(b) / 255.0f,
            static_cast<float>(a) / 255.0f
        };
        return true;
    }

    bool SameColor(const Vector4& a, const Vector4& b)
    {
        return a.x == b.x &&
            a.y == b.y &&
            a.z == b.z &&
            a.w == b.w;
    }
}

void Text::Initialize(const std::string& fontPath, uint32_t pixelSize, const std::u32string& text)
{
    textManager_ = TextManager::GetInstance();

    fontKey_ =
    {
        fontPath,
        pixelSize,
        false,
        false
    };
    fontStylePaths_.regular = fontPath;
    lineHeight_ = static_cast<float>(pixelSize);

    fontAtlas_ = FontManager::GetInstance()->LoadFont(fontKey_);

    CreateVertexResource();
    CreateMaterialResource();

    worldTransform_.Initialize();
    worldTransform_.translate_ = Vector3{ position_.x, position_.y, 0.0f };

    SetText(text);
}

void Text::InitializeRichText(const std::string& fontPath, uint32_t pixelSize, const std::u32string& text)
{
    Initialize(fontPath, pixelSize, U"");
    SetRichText(text);
}

void Text::Update()
{
#ifdef _DEBUG

    ImGui::Begin("Text Setting");

    Vector3 newTranslate = worldTransform_.translate_;
    if (ImGui::DragFloat3("translate", &newTranslate.x, 0.1f))
    {
        worldTransform_.translate_ = newTranslate;
        position_ = { newTranslate.x, newTranslate.y };
    }

    Vector4 newColor = materialData_->color;
    if (ImGui::ColorEdit4("color", &newColor.x))
    {
        materialData_->color = newColor;
    }

    ImGui::End();

#endif

    worldTransform_.SetRotate(Vector3{ 0.0f, 0.0f, 0.0f });
    worldTransform_.scale_ = Vector3{ 1.0f, 1.0f, 1.0f };

    worldTransform_.UpdateMatrix();

    Matrix4x4 viewMatrix = MakeIdentity4x4();
    Matrix4x4 projectionMatrix = MakeOrthographicMatrix(
        0.0f, 0.0f, float(WindowsAPI::kClientWidth), float(WindowsAPI::kClientHeight), 0.0f, 100.0f);
    Matrix4x4 viewProjectionMatrix = Multiply(viewMatrix, projectionMatrix);

    worldTransform_.TransferMatrix(viewProjectionMatrix);
}

void Text::Draw()
{
    if (currentIndexCount_ == 0 || drawBatches_.empty())
    {
        return;
    }

    auto psoSet = PSOManager::GetInstance()->GetPSOData(
       textManager_->GetDefaultPsoName(), textManager_->GetDefaultBlendMode(), PSOManager::FillMode::kSolid);

    auto commandList = DirectXBase::GetInstance()->GetCommandList();
    commandList->SetPipelineState(psoSet.pipelineState.Get());

    commandList->SetGraphicsRootSignature(psoSet.rootSignature.Get());

    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    commandList->IASetIndexBuffer(&indexBufferView_);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    worldTransform_.SetCBufferTransformationResource(1);

    for (const DrawBatch& batch : drawBatches_)
    {
        if (batch.indexCount == 0)
        {
            continue;
        }

        commandList->SetGraphicsRootDescriptorTable(
            2,
            FontManager::GetInstance()->GetSRVHandleGPU(batch.fontKey));

        commandList->DrawIndexedInstanced(batch.indexCount, 1, batch.startIndex, 0, 0);
    }
}

void Text::Finalize()
{

}

void Text::SetText(const std::u32string& text)
{
    text_ = text;
    BuildPlainStyledText(text);
    RebuildMesh();
}

void Text::SetRichText(const std::u32string& text)
{
    text_ = text;
    ParseRichText(text);
    RebuildMesh();
}

void Text::SetFontStylePaths(const FontStylePaths& paths)
{
    fontStylePaths_ = paths;
    if (fontStylePaths_.regular.empty())
    {
        fontStylePaths_.regular = fontKey_.path;
    }
    else
    {
        fontKey_.path = fontStylePaths_.regular;
    }

    fontKey_.syntheticBold = false;
    fontKey_.syntheticItalic = false;

    if (!fontKey_.path.empty() && fontKey_.pixelSize > 0)
    {
        fontAtlas_ = FontManager::GetInstance()->LoadFont(fontKey_);
    }

    if (vertexData_ && indexData_)
    {
        RebuildMesh();
    }
}

void Text::SetFontStylePaths(
    const std::string& regularPath,
    const std::string& boldPath,
    const std::string& italicPath,
    const std::string& boldItalicPath)
{
    SetFontStylePaths(FontStylePaths{
        regularPath,
        boldPath,
        italicPath,
        boldItalicPath
    });
}

void Text::SetPosition(const Vector2& position)
{
    position_ = position;
    worldTransform_.translate_ = Vector3{ position.x, position.y, 0.0f };
}

void Text::SetAnchorPoint(const Vector2& anchorPoint)
{
    anchorPoint_ = anchorPoint;
    RebuildMesh();
}

void Text::CreateVertexResource()
{
    const uint32_t vertexCount = kMaxQuadCount * 4;
    const uint32_t indexCount = kMaxQuadCount * 6;

    vertexResource_ = DirectXBase::GetInstance()->CreateBufferResource(
        sizeof(VertexData) * vertexCount
    );

    indexResource_ = DirectXBase::GetInstance()->CreateBufferResource(
        sizeof(uint32_t) * indexCount
    );

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(VertexData) * vertexCount;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

    indexBufferView_.BufferLocation = indexResource_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = sizeof(uint32_t) * indexCount;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;

    vertexResource_->Map(0, nullptr, reinterpret_cast<void**>(&vertexData_));
    indexResource_->Map(0, nullptr, reinterpret_cast<void**>(&indexData_));
}

void Text::CreateMaterialResource()
{
    materialResource_ = DirectXBase::GetInstance()->CreateBufferResource(sizeof(Material));
    materialResource_->Map(0, nullptr, reinterpret_cast<void**>(&materialData_));

    materialData_->color = Vector4{ 1.0f, 1.0f, 1.0f, 1.0f };
}

void Text::RebuildMesh()
{
    fontAtlas_ = FontManager::GetInstance()->LoadFont(fontKey_);

    if (!vertexData_ || !indexData_)
    {
        return;
    }

    currentIndexCount_ = 0;
    drawBatches_.clear();

    float penX = 0.0f;
    float penY = 0.0f;

    uint32_t quadCount = 0;
    uint32_t displayCharacterCount = 0;

    bool underlineActive = false;
    float underlineStartX = 0.0f;
    float underlineBaselineY = 0.0f;
    FontKey underlineFontKey{};
    FontAtlas* underlineAtlas = nullptr;
    Vector4 underlineColor = { 1.0f, 1.0f, 1.0f, 1.0f };

    auto finishUnderline = [&]()
    {
        if (underlineActive && underlineAtlas)
        {
            AppendUnderlineQuad(
                underlineFontKey,
                *underlineAtlas,
                underlineStartX,
                penX,
                underlineBaselineY,
                underlineColor,
                quadCount);
        }

        underlineActive = false;
        underlineAtlas = nullptr;
    };

    for (const StyledCharacter& styledCharacter : styledCharacters_)
    {
        if (displayCharacterCount >= kMaxTextLength)
        {
            break;
        }

        const char32_t c = styledCharacter.codepoint;

        if (c == U'\n')
        {
            finishUnderline();
            penX = 0.0f;
            penY += lineHeight_;
            ++displayCharacterCount;
            continue;
        }

        if (underlineActive &&
            !styledCharacter.style.underline)
        {
            finishUnderline();
        }

        FontKey drawFontKey{};
        FontAtlas* atlas = LoadAtlasForStyle(styledCharacter.style, drawFontKey);
        if (!atlas)
        {
            continue;
        }

        if (underlineActive &&
            (underlineFontKey != drawFontKey ||
                !SameColor(underlineColor, styledCharacter.style.color)))
        {
            finishUnderline();
        }

        if (!underlineActive && styledCharacter.style.underline)
        {
            underlineActive = true;
            underlineStartX = penX;
            underlineBaselineY = penY + lineHeight_;
            underlineFontKey = drawFontKey;
            underlineAtlas = atlas;
            underlineColor = styledCharacter.style.color;
        }

        auto it = atlas->glyphs.find(c);
        if (it == atlas->glyphs.end())
        {
            it = atlas->glyphs.find(U'?');
            if (it == atlas->glyphs.end())
            {
                ++displayCharacterCount;
                continue;
            }
        }

        const GlyphInfo& glyph = it->second;

        if (glyph.width == 0 || glyph.height == 0)
        {
            penX += static_cast<float>(glyph.advance);
            ++displayCharacterCount;
            continue;
        }

        const float left =
            penX + static_cast<float>(glyph.bearingX);

        const float top =
            penY + (lineHeight_ - static_cast<float>(glyph.bearingY));

        const float right =
            left + static_cast<float>(glyph.width);

        const float bottom =
            top + static_cast<float>(glyph.height);

        AppendQuad(
            drawFontKey,
            { left,  bottom, 0.0f, 1.0f },
            { left,  top,    0.0f, 1.0f },
            { right, bottom, 0.0f, 1.0f },
            { right, top,    0.0f, 1.0f },
            { glyph.uvMin.x, glyph.uvMax.y },
            { glyph.uvMin.x, glyph.uvMin.y },
            { glyph.uvMax.x, glyph.uvMax.y },
            { glyph.uvMax.x, glyph.uvMin.y },
            styledCharacter.style.color,
            quadCount);

        penX += static_cast<float>(glyph.advance);
        ++displayCharacterCount;
    }

    finishUnderline();

    if (quadCount > 0)
    {
        float minX = vertexData_[0].position.x;
        float maxX = vertexData_[0].position.x;
        float minY = vertexData_[0].position.y;
        float maxY = vertexData_[0].position.y;
        const uint32_t vertexCount = quadCount * 4;
        for (uint32_t i = 1; i < vertexCount; ++i)
        {
            minX = std::min(minX, vertexData_[i].position.x);
            maxX = std::max(maxX, vertexData_[i].position.x);
            minY = std::min(minY, vertexData_[i].position.y);
            maxY = std::max(maxY, vertexData_[i].position.y);
        }

        const float anchorX = minX + (maxX - minX) * anchorPoint_.x;
        const float anchorY = minY + (maxY - minY) * anchorPoint_.y;
        for (uint32_t i = 0; i < vertexCount; ++i)
        {
            vertexData_[i].position.x -= anchorX;
            vertexData_[i].position.y -= anchorY;
        }
    }

    currentIndexCount_ = quadCount * 6;
}

void Text::BuildPlainStyledText(const std::u32string& text)
{
    styledCharacters_.clear();
    styledCharacters_.reserve(text.size());

    for (char32_t c : text)
    {
        styledCharacters_.push_back(StyledCharacter{ c, TextStyle{} });
    }
}

void Text::ParseRichText(const std::u32string& text)
{
    styledCharacters_.clear();
    styledCharacters_.reserve(text.size());

    uint32_t boldDepth = 0;
    uint32_t italicDepth = 0;
    uint32_t underlineDepth = 0;
    std::vector<Vector4> colorStack;
    colorStack.push_back({ 1.0f, 1.0f, 1.0f, 1.0f });

    auto currentStyle = [&]() -> TextStyle
    {
        return TextStyle{
            boldDepth > 0,
            italicDepth > 0,
            underlineDepth > 0,
            colorStack.back()
        };
    };

    auto appendCharacter = [&](char32_t c)
    {
        styledCharacters_.push_back(StyledCharacter{ c, currentStyle() });
    };

    size_t i = 0;
    while (i < text.size())
    {
        if (text[i] != U'<')
        {
            appendCharacter(text[i]);
            ++i;
            continue;
        }

        size_t close = i + 1;
        while (close < text.size() && text[close] != U'>')
        {
            ++close;
        }

        if (close >= text.size())
        {
            appendCharacter(text[i]);
            ++i;
            continue;
        }

        const std::u32string tag = text.substr(i + 1, close - i - 1);
        bool recognized = true;

        if (tag == U"b")
        {
            ++boldDepth;
        }
        else if (tag == U"/b" && boldDepth > 0)
        {
            --boldDepth;
        }
        else if (tag == U"i")
        {
            ++italicDepth;
        }
        else if (tag == U"/i" && italicDepth > 0)
        {
            --italicDepth;
        }
        else if (tag == U"u")
        {
            ++underlineDepth;
        }
        else if (tag == U"/u" && underlineDepth > 0)
        {
            --underlineDepth;
        }
        else if (tag.size() > 6 && tag.substr(0, 6) == U"color=")
        {
            Vector4 color{};
            if (TryParseHexColor(tag.substr(6), color))
            {
                colorStack.push_back(color);
            }
            else
            {
                recognized = false;
            }
        }
        else if (tag == U"/color" && colorStack.size() > 1)
        {
            colorStack.pop_back();
        }
        else
        {
            recognized = false;
        }

        if (recognized)
        {
            i = close + 1;
            continue;
        }

        for (size_t literalIndex = i; literalIndex <= close; ++literalIndex)
        {
            appendCharacter(text[literalIndex]);
        }
        i = close + 1;
    }
}

FontAtlas* Text::LoadAtlasForStyle(const TextStyle& style, FontKey& outKey)
{
    const std::string regularPath =
        fontStylePaths_.regular.empty() ? fontKey_.path : fontStylePaths_.regular;

    std::array<FontKey, 5> candidates{};
    size_t candidateCount = 0;

    auto addCandidate = [&](const std::string& path, bool syntheticBold, bool syntheticItalic)
    {
        if (path.empty() || candidateCount >= candidates.size())
        {
            return;
        }

        candidates[candidateCount] = FontKey{
            path,
            fontKey_.pixelSize,
            syntheticBold,
            syntheticItalic
        };
        ++candidateCount;
    };

    if (style.bold && style.italic)
    {
        addCandidate(fontStylePaths_.boldItalic, false, false);
        addCandidate(fontStylePaths_.bold, false, true);
        addCandidate(fontStylePaths_.italic, true, false);
        addCandidate(regularPath, true, true);
    }
    else if (style.bold)
    {
        addCandidate(fontStylePaths_.bold, false, false);
        addCandidate(regularPath, true, false);
    }
    else if (style.italic)
    {
        addCandidate(fontStylePaths_.italic, false, false);
        addCandidate(regularPath, false, true);
    }
    else
    {
        addCandidate(regularPath, false, false);
    }

    for (size_t i = 0; i < candidateCount; ++i)
    {
        FontAtlas* atlas = FontManager::GetInstance()->LoadFont(candidates[i]);
        if (atlas)
        {
            outKey = candidates[i];
            return atlas;
        }
    }

    return nullptr;
}

bool Text::AppendQuad(
    const FontKey& fontKey,
    const Vector4& leftBottom,
    const Vector4& leftTop,
    const Vector4& rightBottom,
    const Vector4& rightTop,
    const Vector2& leftBottomUv,
    const Vector2& leftTopUv,
    const Vector2& rightBottomUv,
    const Vector2& rightTopUv,
    const Vector4& color,
    uint32_t& quadCount)
{
    if (quadCount >= kMaxQuadCount)
    {
        return false;
    }

    const uint32_t vertexBase = quadCount * 4;
    const uint32_t indexBase = quadCount * 6;

    vertexData_[vertexBase + 0].position = leftBottom;
    vertexData_[vertexBase + 1].position = leftTop;
    vertexData_[vertexBase + 2].position = rightBottom;
    vertexData_[vertexBase + 3].position = rightTop;

    vertexData_[vertexBase + 0].texcoord = leftBottomUv;
    vertexData_[vertexBase + 1].texcoord = leftTopUv;
    vertexData_[vertexBase + 2].texcoord = rightBottomUv;
    vertexData_[vertexBase + 3].texcoord = rightTopUv;

    vertexData_[vertexBase + 0].normal = { 0.0f, 0.0f, -1.0f };
    vertexData_[vertexBase + 1].normal = { 0.0f, 0.0f, -1.0f };
    vertexData_[vertexBase + 2].normal = { 0.0f, 0.0f, -1.0f };
    vertexData_[vertexBase + 3].normal = { 0.0f, 0.0f, -1.0f };

    vertexData_[vertexBase + 0].color = color;
    vertexData_[vertexBase + 1].color = color;
    vertexData_[vertexBase + 2].color = color;
    vertexData_[vertexBase + 3].color = color;

    indexData_[indexBase + 0] = vertexBase + 0;
    indexData_[indexBase + 1] = vertexBase + 1;
    indexData_[indexBase + 2] = vertexBase + 2;
    indexData_[indexBase + 3] = vertexBase + 1;
    indexData_[indexBase + 4] = vertexBase + 3;
    indexData_[indexBase + 5] = vertexBase + 2;

    AppendDrawBatch(fontKey, indexBase, 6);

    ++quadCount;
    return true;
}

void Text::AppendDrawBatch(const FontKey& fontKey, uint32_t startIndex, uint32_t indexCount)
{
    if (!drawBatches_.empty())
    {
        DrawBatch& lastBatch = drawBatches_.back();
        if (lastBatch.fontKey == fontKey &&
            lastBatch.startIndex + lastBatch.indexCount == startIndex)
        {
            lastBatch.indexCount += indexCount;
            return;
        }
    }

    drawBatches_.push_back(DrawBatch{
        fontKey,
        startIndex,
        indexCount
    });
}

void Text::AppendUnderlineQuad(
    const FontKey& fontKey,
    const FontAtlas& atlas,
    float startX,
    float endX,
    float baselineY,
    const Vector4& color,
    uint32_t& quadCount)
{
    if (endX <= startX)
    {
        return;
    }

    const float centerY = baselineY + atlas.underlineOffset;
    const float halfThickness = atlas.underlineThickness * 0.5f;
    const float top = centerY - halfThickness;
    const float bottom = centerY + halfThickness;
    const Vector2 uv = atlas.solidUv;

    AppendQuad(
        fontKey,
        { startX, bottom, 0.0f, 1.0f },
        { startX, top,    0.0f, 1.0f },
        { endX,   bottom, 0.0f, 1.0f },
        { endX,   top,    0.0f, 1.0f },
        uv,
        uv,
        uv,
        uv,
        color,
        quadCount);
}
