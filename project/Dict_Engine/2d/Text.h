#pragma once
#include "TextManager.h"
#include "FontManager.h"
#include "WorldTransform.h"

#include <string>
#include <vector>

class Text
{
public:

    struct FontStylePaths
    {
        std::string regular;
        std::string bold;
        std::string italic;
        std::string boldItalic;
    };

    struct VertexData
    {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
        Vector4 color;
    };

    struct Material
    {
        Vector4 color;
    };

    struct TransformationMatrix
    {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };

public:

    void Initialize(const std::string& fontPath, uint32_t pixelSize, const std::u32string& text);
    void InitializeRichText(const std::string& fontPath, uint32_t pixelSize, const std::u32string& text);
    void Update();
    void Draw();
    void Finalize();

public: // 外部入力＆出力

    void SetText(const std::u32string& text); // 変更時のみレイアウト再構築
    
    void SetRichText(const std::u32string& text);
    void SetFontStylePaths(const FontStylePaths& paths);
    void SetFontStylePaths(
        const std::string& regularPath,
        const std::string& boldPath = "",
        const std::string& italicPath = "",
        const std::string& boldItalicPath = "");

    void SetPosition(const Vector2& position);
    void SetAnchorPoint(const Vector2& anchorPoint);
    void SetColor(const Vector4& color) { materialData_->color = color; }

    const Vector2 GetPosition(){ return position_; }
    const Vector2& GetAnchorPoint() const { return anchorPoint_; }
    const Vector4& GetColor() const { return materialData_->color; }

private:

    struct TextStyle
    {
        bool bold = false;
        bool italic = false;
        bool underline = false;
        Vector4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct StyledCharacter
    {
        char32_t codepoint = U'\0';
        TextStyle style{};
    };

    struct DrawBatch
    {
        FontKey fontKey{};
        uint32_t startIndex = 0;
        uint32_t indexCount = 0;
    };

private:

    void CreateVertexResource(); // kMaxTextLength分を確保
    void CreateMaterialResource();
    void RebuildMesh(); // text_からvertexData_を再構築、indexCount_を更新

    void BuildPlainStyledText(const std::u32string& text);
    void ParseRichText(const std::u32string& text);
    FontAtlas* LoadAtlasForStyle(const TextStyle& style, FontKey& outKey);
    bool AppendQuad(
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
        uint32_t& quadCount);
    void AppendDrawBatch(const FontKey& fontKey, uint32_t startIndex, uint32_t indexCount);
    void AppendUnderlineQuad(
        const FontKey& fontKey,
        const FontAtlas& atlas,
        float startX,
        float endX,
        float baselineY,
        const Vector4& color,
        uint32_t& quadCount);

    static constexpr uint32_t kMaxTextLength = 256;
    static constexpr uint32_t kMaxQuadCount = kMaxTextLength * 2;

private:

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    VertexData* vertexData_ = nullptr;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
    uint32_t* indexData_ = nullptr;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    uint32_t currentIndexCount_ = 0; // 実際の文字数×6

    std::vector<DrawBatch> drawBatches_;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;


    TextManager* textManager_ = nullptr;
    FontAtlas* fontAtlas_ = nullptr; // FontManagerから取得した参照
    FontKey fontKey_{};
    FontStylePaths fontStylePaths_{};

    WorldTransform worldTransform_;
    Vector2 position_ = { 0.0f, 0.0f };
    Vector2 anchorPoint_ = { 0.0f, 0.0f };
    float lineHeight_ = 32.0f;

    std::u32string text_;
    std::vector<StyledCharacter> styledCharacters_;
};

