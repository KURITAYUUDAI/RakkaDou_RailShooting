struct Well
{
	row_major float4x4 skeletonSpaceMatrix;
	row_major float4x4 skeletonSpaceInverseTransposeMatrix;
};

struct Vertex
{
	float32_t4 position;
	float32_t2 texcoord;
	float32_t3 normal;
};

struct VertexInfluence
{
	float32_t4 weight;
	int32_t4 index;
};

struct SkinningInformation
{
	uint32_t numVertices;
};

// SkinningObject3d.VS.hlslで作ったものと同じPalette
StructuredBuffer<Well> gMatrixPalette : register(t0);
// VertexBufferViewのstream0として利用していた入力頂点
StructuredBuffer<Vertex> gInputVertices : register(t1);
// VertexBufferViewのstream1として利用していた入力インフルエンス
StructuredBuffer<VertexInfluence> gInfluences : register(t2);
// Skinning計算後の頂点データ。SkinnedVertex
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);
// Skinningに関するちょっとした情報
ConstantBuffer<SkinningInformation> gSkinningInformation : register(b0);

struct Skinned
{
	float4 position;
	float3 normal;
};

Skinned Skinning(Vertex input, VertexInfluence influence)
{
	Skinned skinned;
    
    // 位置の変換
	skinned.position = mul(input.position, gMatrixPalette[influence.index.x].skeletonSpaceMatrix) * influence.weight.x;
	skinned.position += mul(input.position, gMatrixPalette[influence.index.y].skeletonSpaceMatrix) * influence.weight.y;
	skinned.position += mul(input.position, gMatrixPalette[influence.index.z].skeletonSpaceMatrix) * influence.weight.z;
	skinned.position += mul(input.position, gMatrixPalette[influence.index.w].skeletonSpaceMatrix) * influence.weight.w;
	skinned.position.w = 1.0f; // 確実に1を入れる
    
    // 法線の変換
	skinned.normal = mul(input.normal, (float3x3) gMatrixPalette[influence.index.x].skeletonSpaceInverseTransposeMatrix) * influence.weight.x;
	skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[influence.index.y].skeletonSpaceInverseTransposeMatrix) * influence.weight.y;
	skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[influence.index.z].skeletonSpaceInverseTransposeMatrix) * influence.weight.z;
	skinned.normal += mul(input.normal, (float3x3) gMatrixPalette[influence.index.w].skeletonSpaceInverseTransposeMatrix) * influence.weight.w;
	skinned.normal = normalize(skinned.normal); // 正規化して戻してあげる
    
	return skinned;
}

[numthreads(1024, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	uint32_t vertexIndex = DTid.x;
	if (vertexIndex >= gSkinningInformation.numVertices)
	{
		// Skinning計算	
		Vertex input = gInputVertices[vertexIndex];
		VertexInfluence influence = gInfluences[vertexIndex];
		
		// Skinning後の頂点を計算
		Vertex skinnedVertex;
		skinnedVertex.texcoord = input.texcoord;
		
		Skinned skinned = Skinning(input, influence);
		skinnedVertex.position = skinned.position;
		skinnedVertex.normal = skinned.normal;
		
		// Skinning後の頂点データを格納、つまり書き込む
		gOutputVertices[vertexIndex] = skinnedVertex;
		
	}
}