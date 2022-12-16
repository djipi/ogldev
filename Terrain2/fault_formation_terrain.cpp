#include "fault_formation_terrain.h"

void FaultFormationTerrain::CreateFaultFormation(int Size, int Iterations, float MinHeight, float MaxHeight, float Filter)
{
    if (MinHeight >= MaxHeight) {
        printf("%s: MinHeight (%f) must be less-than MaxHeight (%f\n)", __FUNCTION__, MinHeight, MaxHeight);
        assert(0);
    }

    m_terrainSize = Size;
    m_minHeight = MinHeight;
    m_maxHeight = MaxHeight;

    m_heightMap.InitArray2D(Size, Size, 0.0f);

    CreateFaultFormationF32(Iterations, MinHeight, MaxHeight, Filter);

    m_heightMap.Normalize(MinHeight, MaxHeight);

    m_triangleList.CreateTriangleList(m_terrainSize, m_terrainSize, this);
}


void FaultFormationTerrain::CreateFaultFormationF32(int Iterations, float MinHeight, float MaxHeight, float Filter)
{
    float DeltaHeight = (float)MaxHeight - (float)MinHeight;

    for (int CurIter = 0 ; CurIter < Iterations ; CurIter++) {
        float Height = (float)MaxHeight - DeltaHeight * ((float)CurIter / (float)Iterations);

        TerrainPoint p1, p2;

        GenRandomTerrainPoints(p1, p2);

        int DirX = p2.x - p1.x;
        int DirZ = p2.z - p1.z;

        printf("Iter %d Height %f\n", CurIter, Height);
#ifdef DEBUG_PRINT
        printf("Random points: "); p1.Print(); printf(" --> "); p2.Print(); printf("\n");
        printf("   ");
        for (int i = 0 ; i < m_terrainSize ; i++) {
            printf("%4d ", i);
        }
        printf("\n");
#endif

        for (int z = 0 ; z < m_terrainSize ; z++) {
#ifdef DEBUG_PRINT
            printf("%d: ", z);
#endif
            for (int x = 0 ; x < m_terrainSize ; x++) {
                int DirX_in = x - p1.x;
                int DirZ_in = z - p1.z;

                int CrossProduct = DirX_in * DirZ - DirX * DirZ_in;
#ifdef DEBUG_PRINT
                //                printf("%4d ", CrossProduct);
#endif
                if (CrossProduct > 0) {
                    float CurHeight = m_heightMap.Get(x, z);
                    m_heightMap.Set(x, z, CurHeight + Height);
                }

#ifdef DEBUG_PRINT
                printf("%f ", m_heightMap.Get(x, z));
#endif
            }
#ifdef DEBUG_PRINT
            printf("\n");
#endif
            ApplyFIRFilter(Filter);
        }
    }
}


void FaultFormationTerrain::ApplyFIRFilter(float Filter)
{
    // left to right
    for (int y = 0 ; y < m_terrainSize ; y++) {
        float PrevFractalVal = m_heightMap.Get(0, y);
        for (int x = 1 ; x < m_terrainSize ; x++) {
            float CurFractalVal = m_heightMap.Get(x, y);
            float NewVal = Filter * PrevFractalVal + (1 - Filter) * CurFractalVal;
            m_heightMap.Set(x, y, NewVal);
            PrevFractalVal = NewVal;
        }
    }

    // right to left
    for (int y = 0 ; y < m_terrainSize ; y++) {
        float PrevFractalVal = m_heightMap.Get(m_terrainSize - 1, y);
        for (int x = m_terrainSize - 2 ; x >= 0 ; x--) {
            float CurFractalVal = m_heightMap.Get(x, y);
            float NewVal = Filter * PrevFractalVal + (1 - Filter) * CurFractalVal;
            m_heightMap.Set(x, y, NewVal);
            PrevFractalVal = NewVal;
        }
    }

    // top to bottom
    for (int x = 0 ; x < m_terrainSize ; x++) {
        float PrevFractalVal = m_heightMap.Get(x, 0);
        for (int y = 1 ; y < m_terrainSize ; y++) {
            float CurFractalVal = m_heightMap.Get(x, y);
            float NewVal = Filter * PrevFractalVal + (1 - Filter) * CurFractalVal;
            m_heightMap.Set(x, y, NewVal);
            PrevFractalVal = NewVal;
        }
    }

    // bottom to top
    for (int x = 0 ; x < m_terrainSize ; x++) {
        float PrevFractalVal = m_heightMap.Get(x, m_terrainSize - 1);
        for (int y = m_terrainSize - 2 ; y >= 0 ; y--) {
            float CurFractalVal = m_heightMap.Get(x, y);
            float NewVal = Filter * PrevFractalVal + (1 - Filter) * CurFractalVal;
            m_heightMap.Set(x, y, NewVal);
            PrevFractalVal = NewVal;
        }
    }
}

/*
void FaultFormationTerrain::ApplyFIRFilterBand(Array2D<float>& TempData, int Start, int Stride, int Count, float Filter)
{
        assert(Start < TempData.size());

    float v = TempData[Start];

    for (int i = 1 ; i < Count ; i++) {
        int Index = Start + i * Stride;
        assert(Index < TempData.size());
        TempData[Index] = v * Filter + (1 - Filter) * TempData[Index];
        v = TempData[Index];
        }
}*/


void FaultFormationTerrain::GenRandomTerrainPoints(TerrainPoint& p1, TerrainPoint& p2)
{
    p1.x = rand() % m_terrainSize;
    p1.z = rand() % m_terrainSize;

    int Counter = 0;
    do {
        p2.x = rand() % m_terrainSize;
        p2.z = rand() % m_terrainSize;

        if (Counter++ == 1000) {
            printf("Endless loop detected in %s:%d\n", __FILE__, __LINE__);
            assert(0);
        }
    } while ((p1.x == p2.x) && (p1.z == p2.z));
}
