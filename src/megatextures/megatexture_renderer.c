#include "megatexture_renderer.h"

#define MAX_VERTEX_CACHE_SIZE   32

void megatextureRenderRow(struct MTTileCache* tileCache, struct MTTileLayer* layer, int row, int minX, int maxX, struct RenderState* renderState) {
    int currentVertexCount = 0;

    Gfx* vertexCopyCommand = renderState->dl++;

    struct MTMeshTile* tile = &layer->mesh.tiles[(row << layer->mesh.tileXBits) + minX];
    int startVertex = tile->startVertex;

    for (int x = minX; x < maxX; ++x) {
        int startIndex = tile->startVertex - startVertex;

        if (tile->vertexCount + startIndex > MAX_VERTEX_CACHE_SIZE) {
            // retroactively update the vertex command
            gSPVertex(vertexCopyCommand, &layer->mesh.vertices[startVertex], currentVertexCount, 0);
            vertexCopyCommand = renderState->dl++;
            startIndex = 0;
        }

        currentVertexCount = startIndex + tile->vertexCount;

        Gfx* tileRequest = mtTileCacheRequestTile(tileCache, &layer->tileSource[MT_TILE_WORDS * (x + row * layer->xTiles)], x, row, layer->lod);
        gSPDisplayList(renderState->dl++, tileRequest)

        u8* indices = &layer->mesh.indices[tile->startIndex];
        u8* indexEnd = indices + tile->indexCount;

        for (; indices + 5 < indexEnd; indices += 6) {
            gSP2Triangles(
                renderState->dl++, 
                indices[0] + startIndex, indices[1] + startIndex, indices[2] + startIndex, 0,
                indices[3] + startIndex, indices[4] + startIndex, indices[5] + startIndex, 0
            );
        }

        if (indices + 2 < indexEnd) {
            gSP1Triangle(renderState->dl++, indices[0] + startIndex, indices[1] + startIndex, indices[2] + startIndex, 0);
        }

        ++tile;
    }

    // retroactively update the vertex command
    gSPVertex(vertexCopyCommand, &layer->mesh.vertices[startVertex], currentVertexCount, 0);
}

void megatextureRender(struct MTTileCache* tileCache, struct MTTileIndex* index, struct FrustrumCullingInformation* cullingPlanes, struct RenderState* renderState) {
    if (isOutsideFrustrum(cullingPlanes, &index->boundingBox)) {
        return;
    }

    struct MTTileLayer* topLayer = &index->layers[0];
    for (int row = topLayer->mesh.minTileY; row < topLayer->mesh.maxTileY; ++row) {
        megatextureRenderRow(tileCache, topLayer, row, topLayer->mesh.minTileX, topLayer->mesh.maxTileX, renderState);
    }
}