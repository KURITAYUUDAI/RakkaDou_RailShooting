#define NOMINMAX
#include "FontManager.h"
#include FT_SYNTHESIS_H
#include "SrvManager.h"

#include <algorithm>
#include <cmath>

std::unique_ptr<FontManager> FontManager::instance_ = nullptr;

FontManager* FontManager::GetInstance()
{
    if (instance_ == nullptr)
    {
        instance_ = std::make_unique<FontManager>(ConstructorKey());
    }

    return instance_.get();
}

void FontManager::Finalize()
{
    instance_.reset();
}

FontAtlas* FontManager::LoadFont(
    const std::string& fontPath,
    uint32_t pixelSize,
    bool syntheticBold,
    bool syntheticItalic)
{
    return LoadFont(FontKey{ fontPath, pixelSize, syntheticBold, syntheticItalic });
}

FontAtlas* FontManager::LoadFont(const FontKey& key)
{
	if (atlases_.contains(key))
	{
		return atlases_[key].get();
	}

    FT_Face face = FreeTypeManager::GetInstance()->GetOrLoadFace(key.path);
    if (!face)
    {
        return nullptr;
    }

    // ここが抜けていた
    FT_Error error = FT_Set_Pixel_Sizes(face, 0, key.pixelSize);
    if (error)
    {
        OutputDebugStringA("FT_Set_Pixel_Sizes failed\n");
        return nullptr;
    }

    std::unique_ptr<FontAtlas> newAtlas = std::make_unique<FontAtlas>();

    if (!BuildAsciiFontAtlas(
        face,
        *newAtlas,
        512,
        512,
        key.syntheticBold,
        key.syntheticItalic))
    {
        return nullptr;
    }
    if (!CreateFontAtlasTexture(*newAtlas))
    {
        return nullptr;
    }

    // GPUへのコピーコマンドを確定させる（下記2項も参照）
    DirectXBase::GetInstance()->PostUploadTexture();

    CreateFontAtlasSRV(*newAtlas);
    
    FontAtlas* result = newAtlas.get();
    atlases_[key] = std::move(newAtlas);

	return result;
}

D3D12_GPU_DESCRIPTOR_HANDLE FontManager::GetSRVHandleGPU(const FontKey& key) const
{
	auto it = atlases_.find(key);
	if (it != atlases_.end())
	{
		return it->second->srvHandleGPU;
	}

    assert(false && "GetSRVHandleGPU: 指定されたFontKeyのアトラスが見つかりません");
    return D3D12_GPU_DESCRIPTOR_HANDLE{};

}

bool FontManager::BuildAsciiFontAtlas(
    FT_Face face,
    FontAtlas& outAtlas,
    int atlasWidth,
    int atlasHeight,
    bool syntheticBold,
    bool syntheticItalic)
{
    if (!face)
    {
        return false;
    }

    outAtlas.width = atlasWidth;
    outAtlas.height = atlasHeight;
    outAtlas.pixels.clear();
    outAtlas.pixels.resize(static_cast<size_t>(atlasWidth) * atlasHeight, 0);
    outAtlas.glyphs.clear();
    outAtlas.solidUv = {
        0.5f / static_cast<float>(atlasWidth),
        0.5f / static_cast<float>(atlasHeight)
    };
    outAtlas.pixels[0] = 255;

    const float pixelSize =
        face->size ? static_cast<float>(face->size->metrics.y_ppem) : 32.0f;
    float underlinePosition = 0.0f;
    float underlineThickness = 0.0f;
    if (face->size)
    {
        underlinePosition =
            static_cast<float>(FT_MulFix(face->underline_position, face->size->metrics.y_scale)) / 64.0f;
        underlineThickness =
            std::abs(static_cast<float>(FT_MulFix(face->underline_thickness, face->size->metrics.y_scale)) / 64.0f);
    }
    outAtlas.underlineOffset =
        underlinePosition < 0.0f ? -underlinePosition : std::max(1.0f, pixelSize * 0.1f);
    outAtlas.underlineThickness =
        underlineThickness >= 1.0f ? underlineThickness : std::max(1.0f, pixelSize * 0.06f);

    const int padding = 1;

    int penX = padding;
    int penY = padding;
    int rowHeight = 0;

    for (char32_t c = U' '; c <= U'~'; ++c)
    {
        FT_Error error = FT_Load_Char(
            face,
            static_cast<FT_ULong>(c),
            FT_LOAD_DEFAULT
        );

        if (error)
        {
            continue;
        }

        if (syntheticItalic)
        {
            FT_GlyphSlot_Oblique(face->glyph);
        }
        if (syntheticBold)
        {
            FT_GlyphSlot_Embolden(face->glyph);
        }

        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (error)
        {
            continue;
        }

        FT_GlyphSlot glyph = face->glyph;
        const FT_Bitmap& bitmap = glyph->bitmap;

        const int glyphWidth = static_cast<int>(bitmap.width);
        const int glyphHeight = static_cast<int>(bitmap.rows);

        // 横に入りきらないなら次の行へ
        if (penX + glyphWidth + padding > atlasWidth)
        {
            penX = padding;
            penY += rowHeight + padding;
            rowHeight = 0;
        }

        // 縦に入りきらないなら失敗
        if (penY + glyphHeight + padding > atlasHeight)
        {
            return false;
        }

        GlyphInfo info;
        info.width = glyphWidth;
        info.height = glyphHeight;
        info.bearingX = glyph->bitmap_left;
        info.bearingY = glyph->bitmap_top;
        info.advance = static_cast<int>(glyph->advance.x >> 6);

        info.atlasX = penX;
        info.atlasY = penY;

        // bitmapをatlasへコピー
        if (bitmap.buffer && glyphWidth > 0 && glyphHeight > 0)
        {
            for (int y = 0; y < glyphHeight; ++y)
            {
                for (int x = 0; x < glyphWidth; ++x)
                {
                    const int dstX = penX + x;
                    const int dstY = penY + y;

                    const size_t dstIndex =
                        static_cast<size_t>(dstY) * atlasWidth + dstX;

                    const size_t srcIndex =
                        static_cast<size_t>(y) * std::abs(bitmap.pitch) + x;

                    outAtlas.pixels[dstIndex] = bitmap.buffer[srcIndex];
                }
            }
        }

        info.uvMin = {
            static_cast<float>(penX) / static_cast<float>(atlasWidth),
            static_cast<float>(penY) / static_cast<float>(atlasHeight)
        };

        info.uvMax = {
            static_cast<float>(penX + glyphWidth) / static_cast<float>(atlasWidth),
            static_cast<float>(penY + glyphHeight) / static_cast<float>(atlasHeight)
        };

        outAtlas.glyphs[c] = info;

        penX += glyphWidth + padding;
        rowHeight = std::max(rowHeight, glyphHeight);
    }

    return true;
}

bool FontManager::CreateFontAtlasTexture(FontAtlas& atlas)
{
    if (atlas.pixels.empty() || atlas.width <= 0 || atlas.height <= 0)
    {
        return false;
    }

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Width = static_cast<UINT64>(atlas.width);
    textureDesc.Height = static_cast<UINT>(atlas.height);
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.Format = DXGI_FORMAT_R8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    HRESULT hr = DirectXBase::GetInstance()->GetDevice()->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&atlas.textureResource)
    );

    if (FAILED(hr))
    {
        return false;
    }

    UINT64 uploadBufferSize = 0;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT numRows = 0;
    UINT64 rowSizeInBytes = 0;

    DirectXBase::GetInstance()->GetDevice()->GetCopyableFootprints(
        &textureDesc,
        0,
        1,
        0,
        &footprint,
        &numRows,
        &rowSizeInBytes,
        &uploadBufferSize
    );

    D3D12_HEAP_PROPERTIES uploadHeap{};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC uploadDesc{};
    uploadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    uploadDesc.Width = uploadBufferSize;
    uploadDesc.Height = 1;
    uploadDesc.DepthOrArraySize = 1;
    uploadDesc.MipLevels = 1;
    uploadDesc.Format = DXGI_FORMAT_UNKNOWN;
    uploadDesc.SampleDesc.Count = 1;
    uploadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    hr = DirectXBase::GetInstance()->GetDevice()->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&atlas.uploadResource)
    );

    if (FAILED(hr))
    {
        return false;
    }

    uint8_t* mappedData = nullptr;

    hr = atlas.uploadResource->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&mappedData)
    );

    if (FAILED(hr))
    {
        return false;
    }

    const uint8_t* src = atlas.pixels.data();
    uint8_t* dst = mappedData + footprint.Offset;

    const size_t srcRowPitch = static_cast<size_t>(atlas.width);
    const size_t dstRowPitch = static_cast<size_t>(footprint.Footprint.RowPitch);

    for (int y = 0; y < atlas.height; ++y)
    {
        std::memcpy(
            dst + static_cast<size_t>(y) * dstRowPitch,
            src + static_cast<size_t>(y) * srcRowPitch,
            srcRowPitch
        );
    }

    atlas.uploadResource->Unmap(0, nullptr);

    D3D12_TEXTURE_COPY_LOCATION dstLocation{};
    dstLocation.pResource = atlas.textureResource.Get();
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLocation{};
    srcLocation.pResource = atlas.uploadResource.Get();
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    srcLocation.PlacedFootprint = footprint;

    DirectXBase::GetInstance()->GetCommandList()->CopyTextureRegion(
        &dstLocation,
        0,
        0,
        0,
        &srcLocation,
        nullptr
    );

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = atlas.textureResource.Get();
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    DirectXBase::GetInstance()->GetCommandList()->ResourceBarrier(1, &barrier);

    return true;
}

bool FontManager::CreateFontAtlasSRV(FontAtlas & atlas)
{
    atlas.srvIndex = SrvManager::GetInstance()->AllocateIndex();
    atlas.srvHandleCPU = SrvManager::GetInstance()->GetCPUDescriptorHandle(atlas.srvIndex);
    atlas.srvHandleGPU = SrvManager::GetInstance()->GetGPUDescriptorHandle(atlas.srvIndex);

    SrvManager::GetInstance()->CreateSRVforFontAtlas(
        atlas.srvIndex,
        atlas.textureResource.Get(),
        1,
        0
    );

    return true;
}
