#include "atgBase.h"
#include "atgRenderer.h"
#include "atgShaderLibrary.h"
#include "atgProfile.h"

extern void __atgSetVSyncState(bool enable);

//>nvidia卡优先使用
//extern "C" {
//    _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
//}

atgRenderer* g_Renderer = NULL;

const char* StringOpenGLIdentity = "OpenGL Renderer";
const char* StringGLESIdentity = "OpenGL GLES Renderer(2.0)";
const char* StringDirectxIdentity = "Directx Renderer 9.0";

#define FVCC(a, b, c, d) (uint32)( ((uint32)d) + (((uint32)c)<<8) + (((uint32)b)<<16) + (((uint32)a)<<24) )

bool IsOpenGLGraph()
{
#if defined (USE_OPENGL)
    return true;
#else
    return false;
#endif // USE_OPENGL
}



bool atgRenderer_Private_BeginLine(std::vector<float>& drawLines)
{
    drawLines.clear();
    return true;
}

void atgRenderer_Private_AddLine(std::vector<float>& drawLines, const atgVec3& point1, const atgVec3& point2, const atgVec3& color)
{
    drawLines.push_back(point1.x);
    drawLines.push_back(point1.y);
    drawLines.push_back(point1.z);
    drawLines.push_back(color.x);
    drawLines.push_back(color.y);
    drawLines.push_back(color.z);
    drawLines.push_back(point2.x);
    drawLines.push_back(point2.y);
    drawLines.push_back(point2.z);
    drawLines.push_back(color.x);
    drawLines.push_back(color.y);
    drawLines.push_back(color.z);
}

void  atgRenderer_Private_EndLine(std::vector<float>& drawLines)
{
    if (!drawLines.empty())
    {
        ATG_PROFILE("atgRenderer_Private_EndLine");

        static const int dataCount = 12;
        static atgPass* pColorPass = NULL;
        static atgVertexBuffer* pVB = NULL;

        const std::vector<float>& lines = drawLines;
        if (lines.size() < dataCount)
            return;

        int lineCount = lines.size() / dataCount;
        size_t sizeOfLineData = lines.size() * sizeof(float);

        // create pass
        if (!pColorPass || pColorPass->IsLost())
        {
            pColorPass = atgShaderLibFactory::FindOrCreatePass(VERTEXCOLOR_PASS_IDENTITY);
            if (NULL == pColorPass)
                return;
        }

        // create vertex buffer
        if (!pVB || pVB->IsLost() || sizeOfLineData > pVB->GetSize())
        {
            if (pVB)
            {
                SAFE_DELETE(pVB);
            }

            atgVertexDecl decl;
            decl.AppendElement(0, atgVertexDecl::VA_Pos3);
            decl.AppendElement(0, atgVertexDecl::VA_Diffuse);
            pVB = new atgVertexBuffer();
            pVB->Create(&decl, &lines[0], sizeOfLineData, BAM_Dynamic);
        }else
        {
            // only update vertex buffer
            void *pLockData = pVB->Lock(0, sizeOfLineData);
            if(pLockData)
            {
                memcpy(pLockData, &lines[0], sizeOfLineData);
                pVB->Unlock();
            }
        }
        
        //g_Renderer->SetLightEnable(false);
        //g_Renderer->SetDepthTestEnable(false);
        pColorPass->Bind();
        pVB->Bind();
        g_Renderer->DrawPrimitive(PT_LINES, lineCount, lineCount*2);
        pVB->Unbind();
        pColorPass->Unbind();
        //g_Renderer->SetDepthTestEnable(true);
        //g_Renderer->SetLightEnable(true);

    }
}


void atgGpuResource::Lost()
{
    _isLost = true;
}

void atgGpuResource::ReSet()
{
    _isLost = false;
}

atgRenderer::atgRenderer(atgGame* game)
{
    _game = game;

    for(int i = 0; i < MD_NUMBER; ++i)
        _matrixs[i].Identity();

    _globalAmbientColor = atgVec3(0.03f, 0.03f, 0.03f);

    _VP_offsetX = 0;
    _VP_offsetY = 0;
    _VP_width = 0;
    _VP_height = 0;

    _cullMode = FCM_CW;

    //LOG("atgRenderer _CommonInitialized!\n");
}

atgRenderer::~atgRenderer(void)
{

}

#if defined (WIN32)
#if defined (USE_OPENGL) 

const char* atgRenderer::getName()
{
#ifndef USE_OPENGLES
    return StringOpenGLIdentity;
#else
    return StringGLESIdentity;
#endif
}

#elif defined(USE_DIRECTX)

const char* atgRenderer::getName()
{
    return StringDirectxIdentity;
}

#endif

#elif defined(_ANDROID)

const char* atgRenderer::getName()
{
    return StringGLESIdentity;
}

#endif

void atgRenderer::SetVSyncState(bool sync)
{
    if (sync)
    {
        LOG("-----vsync on-----\n");
    }
    else
    {
        LOG("-----vsync off-----\n");
    }
    ::__atgSetVSyncState(sync);
}

void atgRenderer::AddBindLight(atgLight* light)
{
#ifdef _DEBUG
    bindLights::iterator it = _bindLights.begin();
    for (bindLights::iterator end = _bindLights.end(); it != end; ++it)
    {
        if (*it == light)
        {
            AASSERT(0);
            return;
        }
    }
#endif

    _bindLights.push_back(light);
}

void atgRenderer::AddBindLights( const bindLights& lights )
{
    std::copy(lights.begin(), lights.end(), _bindLights.begin());
}

void atgRenderer::RemoveBindLight(atgLight* light)
{
    bindLights::iterator it = _bindLights.begin();
    for (bindLights::iterator end = _bindLights.end(); it != end; ++it)
    {
        if (*it == light)
        {
            _bindLights.erase(it);
            break;
        }
    }
}

void atgRenderer::ClearBindLight()
{
    _bindLights.clear();
}

bool atgRenderer::BeginPoint()
{
    _drawPoints.clear();
    return true;
}

void atgRenderer::AddPoint(const atgVec3& center, const atgVec3& color)
{
    _drawPoints.push_back(center.x);
    _drawPoints.push_back(center.y);
    _drawPoints.push_back(center.z);
    _drawPoints.push_back(color.x);
    _drawPoints.push_back(color.y);
    _drawPoints.push_back(color.z);
}

void atgRenderer::EndPoint()
{
#ifndef GL_ES_VERSION_2_0

    if(!_drawPoints.empty())
    {
        static const int dataCount = 6;
        static atgPass* pColorPass = NULL;
        static atgVertexBuffer* pVB = NULL;

        const std::vector<float>& points = _drawPoints;
        if (points.size() < dataCount)
            return;

        int pointCount = points.size() / dataCount;
        int sizeOfPointData = points.size() * sizeof(float);

        // create pass
        if (!pColorPass || pColorPass->IsLost())
        {
            pColorPass = atgShaderLibFactory::FindOrCreatePass(VERTEXCOLOR_PASS_IDENTITY);
            if (NULL == pColorPass)
                return;
        }

        // create vertex buffer
        if (!pVB || pVB->IsLost())
        {
            if (pVB)
            {
                SAFE_DELETE(pVB);
            }

            atgVertexDecl decl;
            decl.AppendElement(0, atgVertexDecl::VA_Pos3);
            decl.AppendElement(0, atgVertexDecl::VA_Diffuse);
            pVB = new atgVertexBuffer();
            pVB->Create(&decl, &points[0], sizeOfPointData, BAM_Dynamic);
        }else
        {
            // only update vertex buffer
            void *pLockData = pVB->Lock(0, sizeOfPointData);
            if(pLockData)
            {
                memcpy(pLockData,&points[0], sizeOfPointData);
                pVB->Unlock();
            }
        }

        //g_Renderer->SetDepthTestEnable(false);
        pColorPass->Bind();
        pVB->Bind();

        g_Renderer->DrawPrimitive(PT_POINTS, pointCount, pointCount);
        pVB->Unbind();
        pColorPass->Unbind();
        //g_Renderer->SetDepthTestEnable(true);
    }

#endif
}

bool atgRenderer::BeginLine()
{
    return atgRenderer_Private_BeginLine(_drawLines);
}

void atgRenderer::AddLine(const atgVec3& point1, const atgVec3& point2, const atgVec3& color)
{
    atgRenderer_Private_AddLine(_drawLines, point1, point2, color);
}

void  atgRenderer::EndLine()
{
    atgRenderer_Private_EndLine(_drawLines);
}

//> 使用线队列.比使用线strip速度更快.但是浪费内存,以空间换时间
#define DRAW_QUAD_USE_LINE_LIST

bool atgRenderer::BeginQuad()
{
    _drawQuads.clear();
    return true;
}

void atgRenderer::AddQuad(const atgVec3& point1, const atgVec3& point2, const atgVec3& point3, const atgVec3& point4, const atgVec3& color)
{
    _drawQuads.push_back(point1.x);
    _drawQuads.push_back(point1.y);
    _drawQuads.push_back(point1.z);
    _drawQuads.push_back(color.x);
    _drawQuads.push_back(color.y);
    _drawQuads.push_back(color.z);

    _drawQuads.push_back(point2.x);
    _drawQuads.push_back(point2.y);
    _drawQuads.push_back(point2.z);
    _drawQuads.push_back(color.x);
    _drawQuads.push_back(color.y);
    _drawQuads.push_back(color.z);

#ifdef DRAW_QUAD_USE_LINE_LIST

    _drawQuads.push_back(point2.x);
    _drawQuads.push_back(point2.y);
    _drawQuads.push_back(point2.z);
    _drawQuads.push_back(color.x);
    _drawQuads.push_back(color.y);
    _drawQuads.push_back(color.z);

#endif // DRAW_QUAD_USE_LINE_LIST


    _drawQuads.push_back(point4.x);
    _drawQuads.push_back(point4.y);
    _drawQuads.push_back(point4.z);
    _drawQuads.push_back(color.x);
    _drawQuads.push_back(color.y);
    _drawQuads.push_back(color.z);

#ifdef DRAW_QUAD_USE_LINE_LIST

    _drawQuads.push_back(point4.x);
    _drawQuads.push_back(point4.y);
    _drawQuads.push_back(point4.z);
    _drawQuads.push_back(color.x);
    _drawQuads.push_back(color.y);
    _drawQuads.push_back(color.z);

#endif // DRAW_QUAD_USE_LINE_LIST

    _drawQuads.push_back(point3.x);
    _drawQuads.push_back(point3.y);
    _drawQuads.push_back(point3.z);
    _drawQuads.push_back(color.x);
    _drawQuads.push_back(color.y);
    _drawQuads.push_back(color.z);

#ifdef DRAW_QUAD_USE_LINE_LIST

    _drawQuads.push_back(point3.x);
    _drawQuads.push_back(point3.y);
    _drawQuads.push_back(point3.z);
    _drawQuads.push_back(color.x);
    _drawQuads.push_back(color.y);
    _drawQuads.push_back(color.z);

#endif // DRAW_QUAD_USE_LINE_LIST

    _drawQuads.push_back(point1.x);
    _drawQuads.push_back(point1.y);
    _drawQuads.push_back(point1.z);
    _drawQuads.push_back(color.x);
    _drawQuads.push_back(color.y);
    _drawQuads.push_back(color.z);
}

void atgRenderer::EndQuad()
{
    if (!_drawQuads.empty())
    {
        ATG_PROFILE("atgRenderer::EndQuad");

#ifdef DRAW_QUAD_USE_LINE_LIST
        static const int numVertex = 8;
#else
        static const int numVertex = 5;
#endif // DRAW_QUAD_USE_LINE_LIST

        
        static const int dataCount = 6 * numVertex;
        static atgPass* pColorPass = NULL;
        static atgVertexBuffer* pVB = NULL;

        const std::vector<float>& quads = _drawQuads;
        if (quads.size() < dataCount)
            return;

        int quadCount = quads.size() / dataCount;
        size_t sizeOfQuadData = quads.size() * sizeof(float);

        // create pass
        if (!pColorPass || pColorPass->IsLost())
        {
            pColorPass = atgShaderLibFactory::FindOrCreatePass(VERTEXCOLOR_PASS_IDENTITY);
            if (NULL == pColorPass)
                return;
        }

        // create vertex buffer
        if (!pVB || pVB->IsLost() || sizeOfQuadData > pVB->GetSize())
        {
            if (pVB)
            {
                SAFE_DELETE(pVB);
            }

            atgVertexDecl decl;
            decl.AppendElement(0, atgVertexDecl::VA_Pos3);
            decl.AppendElement(0, atgVertexDecl::VA_Diffuse);
            pVB = new atgVertexBuffer();
            pVB->Create(&decl, &quads[0], sizeOfQuadData, BAM_Dynamic);
        }else
        {
            // only update vertex buffer
            void *pLockData = pVB->Lock(0, sizeOfQuadData);
            if(pLockData)
            {
                memcpy(pLockData, &quads[0], sizeOfQuadData);
                pVB->Unlock();
            }
        }

        //g_Renderer->SetLightEnable(false);
        //g_Renderer->SetDepthTestEnable(false);
        pColorPass->Bind();
        pVB->Bind();

#ifdef DRAW_QUAD_USE_LINE_LIST
        g_Renderer->DrawPrimitive(PT_LINES, quadCount*4, quadCount*numVertex);
#else
        for (int i = 0; i < quadCount; ++i)
        {
            g_Renderer->DrawPrimitive(PT_LINE_STRIP, numVertex - 1, numVertex, i*numVertex);
        }
#endif // DRAW_QUAD_USE_LINE_LIST

        pVB->Unbind();
        pColorPass->Unbind();
        //g_Renderer->SetDepthTestEnable(true);
        //g_Renderer->SetLightEnable(true);
    }
}

bool  atgRenderer::BeginFullQuad()
{
    _drawFullQuads.clear();
    return true;
}

//> 使用三角队列.比使用三角strip速度更快.但是浪费内存,以空间换时间 
#define FULL_QUAD_USE_TRIANGLE_LIST

void  atgRenderer::AddFullQuad(const atgVec3& point1, const atgVec3& point2, const atgVec3& point3, const atgVec3& point4, const atgVec3& color)
{
    _drawFullQuads.push_back(point1.x);
    _drawFullQuads.push_back(point1.y);
    _drawFullQuads.push_back(point1.z);
    _drawFullQuads.push_back(color.x);
    _drawFullQuads.push_back(color.y);
    _drawFullQuads.push_back(color.z);

    _drawFullQuads.push_back(point2.x);
    _drawFullQuads.push_back(point2.y);
    _drawFullQuads.push_back(point2.z);
    _drawFullQuads.push_back(color.x);
    _drawFullQuads.push_back(color.y);
    _drawFullQuads.push_back(color.z);

    _drawFullQuads.push_back(point3.x);
    _drawFullQuads.push_back(point3.y);
    _drawFullQuads.push_back(point3.z);
    _drawFullQuads.push_back(color.x);
    _drawFullQuads.push_back(color.y);
    _drawFullQuads.push_back(color.z);

#ifdef FULL_QUAD_USE_TRIANGLE_LIST

    _drawFullQuads.push_back(point2.x);
    _drawFullQuads.push_back(point2.y);
    _drawFullQuads.push_back(point2.z);
    _drawFullQuads.push_back(color.x);
    _drawFullQuads.push_back(color.y);
    _drawFullQuads.push_back(color.z);

#endif // FULL_QUAD_USE_TRIANGLE_LIST

    _drawFullQuads.push_back(point4.x);
    _drawFullQuads.push_back(point4.y);
    _drawFullQuads.push_back(point4.z);
    _drawFullQuads.push_back(color.x);
    _drawFullQuads.push_back(color.y);
    _drawFullQuads.push_back(color.z);

#ifdef FULL_QUAD_USE_TRIANGLE_LIST

    _drawFullQuads.push_back(point3.x);
    _drawFullQuads.push_back(point3.y);
    _drawFullQuads.push_back(point3.z);
    _drawFullQuads.push_back(color.x);
    _drawFullQuads.push_back(color.y);
    _drawFullQuads.push_back(color.z);

#endif // FULL_QUAD_USE_TRIANGLE_LIST
}

void   atgRenderer::EndFullQuad(const char* pPassIdentity /* = NULL */)
{
    if (!_drawFullQuads.empty())
    {
        ATG_PROFILE("atgRenderer::EndFullQuad");

#ifdef FULL_QUAD_USE_TRIANGLE_LIST
        static const int numVertex = 6;
#else
        static const int numVertex = 4;
#endif // FULL_QUAD_USE_TRIANGLE_LIST

        
        static const int dataCount = 6 * numVertex;
        static atgVertexBuffer* pVB = NULL;

        const std::vector<float>& quads = _drawFullQuads;
        if (quads.size() < dataCount)
            return;

        int quadCount = quads.size() / dataCount;
        size_t sizeOfQuadData = quads.size() * sizeof(float);

        static atgPass* pPass = NULL;
        // create pass
        if (pPassIdentity != NULL)
        {
            pPass = atgShaderLibFactory::FindOrCreatePass(pPassIdentity);
        }else
        {
            pPass = atgShaderLibFactory::FindOrCreatePass(VERTEXCOLOR_PASS_IDENTITY);
        }

        if (pPass == NULL)
        {
            LOG("can't find pass[%s]\n", pPassIdentity ? pPassIdentity : VERTEXCOLOR_PASS_IDENTITY);
            return;
        }

        // create vertex buffer
        if (!pVB || pVB->IsLost() || sizeOfQuadData > pVB->GetSize())
        {
            if (pVB)
            {
                SAFE_DELETE(pVB);
            }

            atgVertexDecl decl;
            decl.AppendElement(0, atgVertexDecl::VA_Pos3);
            decl.AppendElement(0, atgVertexDecl::VA_Diffuse);
            pVB = new atgVertexBuffer();
            pVB->Create(&decl, &quads[0], sizeOfQuadData, BAM_Dynamic);
        }else
        {
            // only update vertex buffer
            void *pLockData = pVB->Lock(0, sizeOfQuadData);
            if(pLockData)
            {
                memcpy(pLockData, &quads[0], sizeOfQuadData);
                pVB->Unlock();
            }
        }

        //g_Renderer->SetLightEnable(false);
        pPass->Bind();
        pVB->Bind();

#ifdef FULL_QUAD_USE_TRIANGLE_LIST
        g_Renderer->DrawPrimitive(PT_TRIANGLES, quadCount*2, quadCount*numVertex);
#else
        for (int i = 0; i < quadCount; ++i)
        {
            g_Renderer->DrawPrimitive(PT_TRIANGLE_STRIP, 2, numVertex, i*numVertex);
        }
#endif // FULL_QUAD_USE_TRIANGLE_LIST

        pVB->Unbind();
        pPass->Unbind();
        //g_Renderer->SetLightEnable(true);
    }
}


bool atgRenderer::DrawTexureQuad(const atgVec3& p1, const atgVec3& p2, const atgVec3& p3, const atgVec3& p4, const atgVec2& t1, const atgVec2& t2, const atgVec2& t3, const atgVec2& t4, atgTexture* pTexture)
{
    AASSERT(pTexture != NULL);

    static float QuadData[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f,

    };
    static const int sizeOfQuadData = sizeof(QuadData);
    static atgPass* pTexturePass = NULL;
    static atgVertexBuffer* pVB = NULL;

    int sizeVec3 = sizeof(atgVec3);
    memcpy(&QuadData[0], p1.m, sizeVec3);
    QuadData[3] = t1.x; QuadData[4] = t1.y;
    memcpy(&QuadData[5], p3.m, sizeVec3);
    QuadData[8] = t3.x; QuadData[9] = t3.y;
    memcpy(&QuadData[10], p2.m, sizeVec3);
    QuadData[13] = t2.x; QuadData[14] = t2.y;
    memcpy(&QuadData[15], p4.m, sizeVec3);
    QuadData[18] = t4.x; QuadData[19] = t4.y;
    // create pass
    if (!pTexturePass || pTexturePass->IsLost())
    {
        pTexturePass = atgShaderLibFactory::FindOrCreatePass(NOT_LIGNTE_TEXTURE_PASS_IDENTITY);
        if (NULL == pTexturePass)
            return false;
        ((atgShaderNotLighteTexture*)pTexturePass)->SetTexture(pTexture);
    }

    // create vertex buffer
    if (!pVB || pVB->IsLost())
    {
        if (pVB)
        {
            SAFE_DELETE(pVB);
        }

        atgVertexDecl decl;
        decl.AppendElement(0, atgVertexDecl::VA_Pos3);
        decl.AppendElement(0, atgVertexDecl::VA_Texture0);

        pVB = new atgVertexBuffer();
        pVB->Create(&decl, QuadData, sizeOfQuadData, BAM_Dynamic);
    }else
    {
        // only update vertex buffer
        void *pLockData = pVB->Lock(0, sizeOfQuadData);
        if(pLockData)
        {
            memcpy(pLockData, QuadData, sizeOfQuadData);
            pVB->Unlock();
        }
    }
    //g_Renderer->SetDepthTestEnable(false);
    pTexture->Bind(0);
    pTexturePass->Bind();
    pVB->Bind();

    g_Renderer->DrawPrimitive(PT_TRIANGLE_STRIP, 2, 4);
    pVB->Unbind();
    pTexturePass->Unbind();
    pTexture->Unbind(0);

    //g_Renderer->SetDepthTestEnable(true);
    return true;
}

bool atgRenderer::DrawFullScreenQuad(atgTexture* pTexture, bool uvConvert/*=flase*/)
{
    AASSERT(pTexture != NULL);
    
    static float QuadData[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
    };
    
    static float QuadDataUVConvert[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    static const int sizeOfQuadData = sizeof(QuadData);
    static atgPass* pTexturePass = NULL;
    static atgVertexBuffer* pVB = NULL;

    // create pass
    if (!pTexturePass || pTexturePass->IsLost())
    {
        pTexturePass = atgShaderLibFactory::FindOrCreatePass(NOT_LIGNTE_TEXTURE_PASS_IDENTITY);
        if (NULL == pTexturePass)
            return false;
        ((atgShaderNotLighteTexture*)pTexturePass)->SetTexture(pTexture);
    }

    // create vertex buffer
    if (!pVB || pVB->IsLost())
    {
        if (pVB)
        {
            SAFE_DELETE(pVB);
        }

        atgVertexDecl decl;
        decl.AppendElement(0, atgVertexDecl::VA_Pos3);
        decl.AppendElement(0, atgVertexDecl::VA_Texture0);

        pVB = new atgVertexBuffer();
        pVB->Create(&decl, uvConvert ? QuadDataUVConvert : QuadData, sizeOfQuadData, BAM_Dynamic);
    }else
    {
        // only update vertex buffer
        void *pLockData = pVB->Lock(0, sizeOfQuadData);
        if(pLockData)
        {
            memcpy(pLockData, uvConvert ? QuadDataUVConvert : QuadData, sizeOfQuadData);
            pVB->Unlock();
        }
    }

    atgMatrix oldWorld;
    g_Renderer->GetMatrix(oldWorld, MD_WORLD);
    g_Renderer->SetMatrix(MD_WORLD, MatrixIdentity);

    atgMatrix oldView;
    g_Renderer->GetMatrix(oldView, MD_VIEW);
    g_Renderer->SetMatrix(MD_VIEW, MatrixIdentity);

    atgMatrix oldProj;
    g_Renderer->GetMatrix(oldProj, MD_PROJECTION);
    //uint32 data[4];
    //g_Renderer->GetViewPort(data[0], data[1], data[2], data[3]);
    //Matrix newProj;
    //atgMath::OrthoProject(data[2], data[3], 0.1f, 100.f, newProj.m);
    //g_Renderer->SetMatrix(MD_PROJECTION, newProj)
    g_Renderer->SetMatrix(MD_PROJECTION, MatrixIdentity);

    //g_Renderer->SetDepthTestEnable(false);
    pTexture->Bind(0);
    pTexturePass->Bind();
    pVB->Bind();
    g_Renderer->DrawPrimitive(PT_TRIANGLE_STRIP, 2, 4);

    pTexture->Unbind(0);
    pTexturePass->Unbind();
    pVB->Unbind();

    //g_Renderer->SetDepthTestEnable(true);

    g_Renderer->SetMatrix(MD_WORLD, oldWorld);
    g_Renderer->SetMatrix(MD_VIEW, oldView);
    g_Renderer->SetMatrix(MD_PROJECTION, oldProj);

    return true;
}

bool atgRenderer::DrawQuadByPass(const atgVec3& p1, const atgVec3& p2, const atgVec3& p3, const atgVec3& p4, const atgVec2& t1, const atgVec2& t2, const atgVec2& t3, const atgVec2& t4, const char* pPassIdentity)
{
    AASSERT(pPassIdentity != NULL);

    atgPass* pPass = atgShaderLibFactory::FindOrCreatePass(pPassIdentity);
    if (pPass == NULL)
    {
        LOG("can't find pass[%s]\n", pPassIdentity);
        return false;
    }

    static float QuadData[] = {
        -1.0f,  1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f,
    };
    static const int sizeOfQuadData = sizeof(QuadData);
    static atgVertexBuffer* pVB = NULL;

    int sizeVec3 = sizeof(atgVec3);
    memcpy(&QuadData[0], p1.m, sizeVec3);
    QuadData[3] = t1.x; QuadData[4] = t1.y;
    memcpy(&QuadData[5], p3.m, sizeVec3);
    QuadData[8] = t3.x; QuadData[9] = t3.y;
    memcpy(&QuadData[10], p2.m, sizeVec3);
    QuadData[13] = t2.x; QuadData[14] = t2.y;
    memcpy(&QuadData[15], p4.m, sizeVec3);
    QuadData[18] = t4.x; QuadData[19] = t4.y;


    // create vertex buffer
    if (!pVB || pVB->IsLost())
    {
        if (pVB)
        {
            SAFE_DELETE(pVB);
        }

        atgVertexDecl decl;
        decl.AppendElement(0, atgVertexDecl::VA_Pos3);
        decl.AppendElement(0, atgVertexDecl::VA_Texture0);

        pVB = new atgVertexBuffer();
        pVB->Create(&decl, QuadData, sizeOfQuadData, BAM_Dynamic);
    }else
    {
        // only update vertex buffer
        void *pLockData = pVB->Lock(0, sizeOfQuadData);
        if(pLockData)
        {
            memcpy(pLockData, QuadData, sizeOfQuadData);
            pVB->Unlock();
        }
    }
    //g_Renderer->SetDepthTestEnable(false);
    pPass->Bind();
    pVB->Bind();

    g_Renderer->DrawPrimitive(PT_TRIANGLE_STRIP, 2, 4);

    pVB->Unbind();
    pPass->Unbind();
    //g_Renderer->SetDepthTestEnable(true);
    return true;
}

bool atgRenderer::BeginAABBoxLine()
{
    return atgRenderer_Private_BeginLine(_drawAABBoxs);
}

void atgRenderer::AddAABBoxLine(const atgVec3& vMin, const atgVec3& vMax, const atgVec3& color)
{
    //  /6 - 7
    // 5 + 2 |
    // | 1 + 8
    // 3 - 4

    atgVec3 a;
    atgVec3 b;

    a.x = vMin.x; a.y = vMin.y; a.z = vMax.z; //>顶点3
    atgRenderer_Private_AddLine(_drawAABBoxs, vMin, a,color);
    a.x = vMin.x; a.y = vMax.y; a.z = vMin.z; //>顶点6
    atgRenderer_Private_AddLine(_drawAABBoxs, vMin, a,color);
    a.x = vMax.x; a.y = vMin.y; a.z = vMin.z; //>顶点8
    atgRenderer_Private_AddLine(_drawAABBoxs, vMin, a,color);

    a.x = vMax.x; a.y = vMax.y; a.z = vMin.z; //>顶点7
    atgRenderer_Private_AddLine(_drawAABBoxs, vMax, a,color);
    a.x = vMax.x; a.y = vMin.y; a.z = vMax.z; //>顶点4
    atgRenderer_Private_AddLine(_drawAABBoxs, vMax, a,color);
    a.x = vMin.x; a.y = vMax.y; a.z = vMax.z; //>顶点5
    atgRenderer_Private_AddLine(_drawAABBoxs, vMax, a,color);

    a.x = vMin.x; a.y = vMin.y; a.z = vMax.z; //>顶点3
    b.x = vMax.x; b.y = vMin.y; b.z = vMax.z; //>顶点4
    atgRenderer_Private_AddLine(_drawAABBoxs, a, b,color);

    a.x = vMin.x; a.y = vMin.y; a.z = vMax.z; //>顶点3
    b.x = vMin.x; b.y = vMax.y; b.z = vMax.z; //>顶点5
    atgRenderer_Private_AddLine(_drawAABBoxs, a, b,color);

    a.x = vMin.x; a.y = vMax.y; a.z = vMax.z; //>顶点5
    b.x = vMin.x; b.y = vMax.y; b.z = vMin.z; //>顶点6
    atgRenderer_Private_AddLine(_drawAABBoxs, a, b,color);

    a.x = vMin.x; a.y = vMax.y; a.z = vMin.z; //>顶点6
    b.x = vMax.x; b.y = vMax.y; b.z = vMin.z; //>顶点7
    atgRenderer_Private_AddLine(_drawAABBoxs, a, b,color);

    a.x = vMax.x; a.y = vMax.y; a.z = vMin.z; //>顶点7
    b.x = vMax.x; b.y = vMin.y; b.z = vMin.z; //>顶点8
    atgRenderer_Private_AddLine(_drawAABBoxs, a, b,color);

    a.x = vMax.x; a.y = vMin.y; a.z = vMax.z; //>顶点4
    b.x = vMax.x; b.y = vMin.y; b.z = vMin.z; //>顶点8
    atgRenderer_Private_AddLine(_drawAABBoxs, a, b,color);
}

void atgRenderer::EndAABBoxLine()
{
    atgRenderer_Private_EndLine(_drawAABBoxs);
}

/*
bool atgRenderer::DrawAABBox(const float vMin[3], const float vMax[3], const float color[3])
{
    //  /4 - 5
    // 0 + 1 |
    // | 6 + 7
    // 2 - 3

    ATG_PROFILE("atgRenderer::DrawAABBox");
    static float AABBoxData[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,

        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
    };

    static uint16 AABBoxIdx1[] = {
        0,2,1,3,5,7,4,6,0,
    };

    static uint16 AABBoxIdx2[] = {
        0,1,5,4,0
    };

    static uint16 AABBoxIdx3[] = {
        2,3,7,6,2
    };

    static const int sizeOfLineData = sizeof(AABBoxData);
    static atgPass* pColorPass = NULL;
    static atgVertexBuffer* pVB = NULL;
    static atgIndexBuffer* pIB1 = NULL;
    static atgIndexBuffer* pIB2 = NULL;
    static atgIndexBuffer* pIB3 = NULL;

    AABBoxData[0] = vMin[0]; AABBoxData[1] = vMax[1]; AABBoxData[2] = vMax[2];
    AABBoxData[3] = vMax[0]; AABBoxData[4] = vMax[1]; AABBoxData[5] = vMax[2];
    AABBoxData[6] = vMin[0]; AABBoxData[7] = vMin[1]; AABBoxData[8] = vMax[2];
    AABBoxData[9] = vMax[0]; AABBoxData[10] = vMin[1]; AABBoxData[11] = vMax[2];

    AABBoxData[12] = vMin[0]; AABBoxData[13] = vMax[1]; AABBoxData[14] = vMin[2];
    AABBoxData[15] = vMax[0]; AABBoxData[16] = vMax[1]; AABBoxData[17] = vMin[2];
    AABBoxData[18] = vMin[0]; AABBoxData[19] = vMin[1]; AABBoxData[20] = vMin[2];
    AABBoxData[21] = vMax[0]; AABBoxData[22] = vMin[1]; AABBoxData[23] = vMin[2];

    // create pass
    if (!pColorPass || pColorPass->IsLost())
    {
        if (pColorPass)
        {
            SAFE_DELETE(pColorPass);
        }

        pColorPass = atgShaderLibFactory::FindOrCreatePass(SOLIDCOLOR_PASS_IDENTITY);
        if (NULL == pColorPass)
            return false;
    }

    if (pColorPass)
    {
        static_cast<atgShaderSolidColor*>(pColorPass)->SetSolidColor(
            Vec3(color[0], color[1], color[2]));
    }

    // create vertex buffer
    if (!pVB || pVB->IsLost())
    {
        if (pVB)
        {
            SAFE_DELETE(pVB);
        }

        atgVertexDecl decl;
        decl.AppendElement(0, atgVertexDecl::VA_Pos3);
        pVB = new atgVertexBuffer();
        pVB->Create(&decl, AABBoxData, sizeOfLineData, BAM_Static);
    }else
    {
        // only update vertex buffer
        void *pLockData = pVB->Lock(0, sizeOfLineData);
        if(pLockData)
        {
            memcpy(pLockData, AABBoxData, sizeOfLineData);
            pVB->Unlock();
        }
    }

    if (pIB1 == NULL)
    {
        pIB1 = new atgIndexBuffer();
        pIB1->Create(AABBoxIdx1, 9, atgIndexBuffer::IFB_Index16, BAM_Static);
    }

    if (pIB2 == NULL)
    {
        pIB2 = new atgIndexBuffer();
        pIB2->Create(AABBoxIdx2, 5, atgIndexBuffer::IFB_Index16, BAM_Static);
    }

    if (pIB3 == NULL)
    {
        pIB3 = new atgIndexBuffer();
        pIB3->Create(AABBoxIdx3, 5, atgIndexBuffer::IFB_Index16, BAM_Static);
    }

    g_Renderer->SetDepthTestEnable(false);
    g_Renderer->BindPass(pColorPass);
    g_Renderer->BindVertexBuffer(pVB);
    g_Renderer->BindIndexBuffer(pIB1);
    g_Renderer->DrawIndexedPrimitive(PT_LINE_STRIP, 8, 9, 8);
    g_Renderer->BindIndexBuffer(pIB2);
    g_Renderer->DrawIndexedPrimitive(PT_LINE_STRIP, 4, 5, 8);
    g_Renderer->BindIndexBuffer(pIB3);
    g_Renderer->DrawIndexedPrimitive(PT_LINE_STRIP, 4, 5, 8);

    g_Renderer->SetDepthTestEnable(true);
    return true;
}
*/

atgPass* atgRenderer::FindCachePass(const char* key)
{
    if (!key)
    {
        return NULL;
    }

    atgPassSet::iterator itr = _cachePass.find(key);
    if (itr != _cachePass.end())
        return itr->second;

    return NULL;
}

atgTexture* atgRenderer::FindCacheTexture(const char* key)
{
    if (!key)
    {
        return NULL;
    }

    atgTextureSet::iterator itr = _cacheTextures.find(key);
    if (itr != _cacheTextures.end())
        return itr->second;

    return NULL;
}

void  atgRenderer::InsertCachePass(const char* Key, atgPass* Pass)
{

#ifdef _DEBUG
    if (FindCachePass(Key))
        return;
#endif // _DEBUG

    _cachePass.insert(atgPassSet::value_type(Key,Pass));

}

void atgRenderer::InsertCacheTexture(const char* Key, atgTexture* TexturePack)
{
#ifdef _DEBUG
    if (FindCacheTexture(Key))
        return;
#endif // _DEBUG

    _cacheTextures.insert(atgTextureSet::value_type(Key, TexturePack));
}

void atgRenderer::PushRenderTarget(uint8 index, atgRenderTarget* pRenderTarget)
{
    if (pRenderTarget)
    {
        _renderTargetStack.push(pRenderTarget);
        pRenderTarget->Bind(index);
    }
}

void atgRenderer::PopRenderTarget(uint8 index)
{
    if (!_renderTargetStack.empty())
    {
        atgRenderTarget* pRenderTarget = _renderTargetStack.top();
        pRenderTarget->Unbind();
        _renderTargetStack.pop();
    }
}

void atgRenderer::InsertGpuResource( atgGpuResource* pRes, GpuResDestoryFunc pFunc )
{
    atgGpuResourceMap::iterator it = _gpuResources.find(pRes);
    if (it != _gpuResources.end())
    {
        return;
    }
    else
    {
        LOG("insert gup resoucse[%s] addr=%p.\n",pRes->GetTypeName(), pRes);
        _gpuResources.insert(std::pair<atgGpuResource*, GpuResDestoryFunc>(pRes, pFunc));
    }
}

void atgRenderer::RemoveGpuResource( atgGpuResource* pRes )
{
    atgGpuResourceMap::iterator it = _gpuResources.find(pRes);
    if (it != _gpuResources.end())
    {
        LOG("remove gup resoucse[%s] addr=%p.\n",it->first->GetTypeName(), it->first);
        _gpuResources.erase(it);
    }
}

void atgRenderer::ReleaseAllGpuResource()
{
    LOG("release all gup resoucse.\n");
    //>释放所有atgGpuResouce
    atgGpuResourceMap::iterator it = _gpuResources.begin();
    for (; it != _gpuResources.end(); ++it)
    {
        (it->first->*it->second)();
        LOG("release gup resoucse[%s] addr=%p.\n",it->first->GetTypeName(), it->first);
    }
}