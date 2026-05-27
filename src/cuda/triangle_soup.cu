#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <iostream>

__global__ void setup_rand_kernel(curandState* state, unsigned long seed, int numTriangles)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx < numTriangles) {
        curand_init(seed, idx, 0, &state[idx]);
    }
}
__global__ void init_triangles_kernel(float3* devVertices, curandState * state, 
                                        float3 center, float3 extents, int numTriangles)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if(idx >= numTriangles) { return; }
    
    curandState localState = state[idx];
    int baseVertexIdx = idx * 3;

    float3 triangleCenter;
    triangleCenter.x = (center.x - extents.x) + curand_uniform(&localState) * (2.0f * extents.x);
    triangleCenter.y = (center.y - extents.y) + curand_uniform(&localState) * (2.0f * extents.y);
    triangleCenter.z = (center.z - extents.z) + curand_uniform(&localState) * (2.0f * extents.z);

    float triangleSize = 0.3f; 

    for(int i = 0; i < 3; i++)
    {
        float offsetX = (curand_uniform(&localState) * 2.0f - 1.0f) * triangleSize;
        float offsetY = (curand_uniform(&localState) * 2.0f - 1.0f) * triangleSize;
        float offsetZ = (curand_uniform(&localState) * 2.0f - 1.0f) * triangleSize;

        float3 pos;
        pos.x = triangleCenter.x + offsetX;
        pos.y = triangleCenter.y + offsetY;
        pos.z = triangleCenter.z + offsetZ;

        devVertices[baseVertexIdx + i] = pos;
    }

    state[idx] = localState;
}

extern "C" void launchTriangleInitialization(
    float3* devVertices, 
    float centerX, float centerY, float centerZ,
    float extentsX, float extentsY, float extentsZ,
    int numTriangles
) {
    curandState* devStates;
    cudaMalloc(&devStates, numTriangles * sizeof(curandState));

    int blockSize = 64;
    int gridSize = (numTriangles + blockSize - 1) / blockSize;

    unsigned long seed = 1234ULL; 
    setup_rand_kernel<<<gridSize, blockSize>>>(devStates, seed, numTriangles);
    cudaDeviceSynchronize();

    float3 center = make_float3(centerX, centerY, centerZ);
    float3 extents = make_float3(extentsX, extentsY, extentsZ);

    init_triangles_kernel<<<gridSize, blockSize>>>(devVertices, devStates, center, extents, numTriangles);
    cudaDeviceSynchronize();

    cudaFree(devStates);
}