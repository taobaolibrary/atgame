#include "atgUtility.h"
#include "atgShaderLibrary.h"
#include "atgMisc.h"
#include "atgCamera.h"
#include "atgIntersection.h"


atgSampleDrawFrustum::atgSampleDrawFrustum()
{

}

atgSampleDrawFrustum::~atgSampleDrawFrustum()
{

}

bool atgSampleDrawFrustum::Create()
{
    _pCamera = new atgCamera();
    _pCamera->SetClipNear(10);
    _pCamera->SetClipFar(400);
    _pCamera->SetPosition(Vec3(0,150,300));

    return true;
}

void atgSampleDrawFrustum::Render()
{
    atgFrustum f;
    f.BuildFrustumPlanes(_pCamera->GetView().m,_pCamera->GetProj().m);
    f.DebugRender();
}

atgSamlpeViewPortDrawAxis::atgSamlpeViewPortDrawAxis()
{

}

atgSamlpeViewPortDrawAxis::~atgSamlpeViewPortDrawAxis()
{

}

void atgSamlpeViewPortDrawAxis::Render( class atgCamera* sceneCamera )
{
    uint32 oldVP[4];
    g_Renderer->GetViewPort(oldVP[0], oldVP[1], oldVP[2], oldVP[3]);

    uint32 newVP[4] = {0, IsOpenGLGraph() ? 0 : oldVP[3] - 100, 100, 100 };
    g_Renderer->SetViewPort(newVP[0], newVP[1], newVP[2], newVP[3]);

    Matrix oldWorld;
    g_Renderer->GetMatrix(oldWorld, MD_WORLD);
    g_Renderer->SetMatrix(MD_WORLD, MatrixIdentity);

    Matrix oldView;
    g_Renderer->GetMatrix(oldView, MD_VIEW);
    Matrix newView(MatrixIdentity);
    Vec3 p(0.0f, 0.0f, 3.5f);
    if (sceneCamera)
    {
        newView.SetColumn3(0, sceneCamera->GetRight());
        newView.SetColumn3(1, sceneCamera->GetUp());
        newView.SetColumn3(2, sceneCamera->GetForward());
        newView.Transpose();
        newView.SetColumn3(3, p*-1.0f);
    }
    else
    {
        atgMath::LookAt(p.m, Vec3Zero.m, Vec3Up.m, newView.m);
    }
    g_Renderer->SetMatrix(MD_VIEW, newView);

    Matrix oldProj;
    g_Renderer->GetMatrix(oldProj, MD_PROJECTION);
    Matrix newProj;
    atgMath::Perspective(37.5f,1.f,0.1f,500.0f,newProj.m);
    g_Renderer->SetMatrix(MD_PROJECTION, newProj);

    g_Renderer->BeginLine();
    g_Renderer->AddLine(Vec3Zero.m, Vec3Up.m, Vec3Up.m);
    g_Renderer->AddLine(Vec3Zero.m, Vec3Right.m, Vec3Right.m);
    g_Renderer->AddLine(Vec3Zero.m, Vec3Forward.m, Vec3Forward.m);
    g_Renderer->EndLine();

    g_Renderer->SetMatrix(MD_WORLD, oldWorld);
    g_Renderer->SetMatrix(MD_VIEW, oldView);
    g_Renderer->SetMatrix(MD_PROJECTION, oldProj);
    g_Renderer->SetViewPort(oldVP[0], oldVP[1], oldVP[2], oldVP[3]);
}

//////////////////////////////////////////////////////////////////////////
//>ˮ��shader
class atgRippleShader : public atgShaderLibPass
{
public:
    atgRippleShader();
    ~atgRippleShader();

    virtual bool			ConfingAndCreate();

    void					SetDxDy(float dx, float dy) { _dx = dx; _dy = dy; }

    void                    SetPrimitiveTex(atgTexture* pTex) { _primitiveTex = pTex; }
    void                    SetRippleTex(atgTexture* pTex) { _rippleTex = pTex; }

protected:
    virtual void			BeginContext(void* data);
protected:
    float                   _dx;    //>�������ĵ���(1/w)
    float                   _dy;    //>������ĵ���(1/h)

    atgTexture*             _primitiveTex;
    atgTexture*             _rippleTex;
};

#define RIPPLE_PASS_IDENTITY "atgRippleShader"
EXPOSE_INTERFACE(atgRippleShader, atgPass, RIPPLE_PASS_IDENTITY);

atgRippleShader::atgRippleShader()
{
    _primitiveTex = NULL;
    _rippleTex = NULL;
}

atgRippleShader::~atgRippleShader()
{
}

bool atgRippleShader::ConfingAndCreate()
{
    bool rs = false;
    if (IsOpenGLGraph())
    {
        rs = atgPass::Create("shaders/ripple.glvs", "shaders/ripple.glfs");
    }
    else
    {
        rs = atgPass::Create("shaders/ripple.dxvs", "shaders/ripple.dxps");
    }

    return rs;
}

void atgRippleShader::BeginContext(void* data)
{
    atgPass::BeginContext(data);
    
    if (_primitiveTex)
    {
        g_Renderer->BindTexture(0, _primitiveTex);
        SetTexture("textureSampler", 0);

        TextureFormat format = _primitiveTex->GetTextureFormat();
        if (format == TF_R4G4B4A4 || 
            format == TF_R8G8B8A8 ||
            format == TF_R5G5B5A1)
        {//>����aplhaͨ��,�������
            g_Renderer->SetBlendFunction(BF_SRC_ALPHA, BF_ONE_MINUS_SRC_ALPHA);
        }
    }

    
    g_Renderer->BindTexture(1, _rippleTex);
    SetTexture("waterHeightSampler", 1);

    Vec4 d(_dx, _dy, 1.0f, 1.0f);
    SetFloat4("u_d", d.m);
}

atgSampleWater::atgSampleWater(void):_pWater(0),_pDyncTexture(0),_pCamera(0)
{

}

atgSampleWater::~atgSampleWater(void)
{
    SAFE_DELETE(_pWater);
    SAFE_DELETE(_pDyncTexture);
    SAFE_DELETE(_pCamera);
}

bool atgSampleWater::Create( int w, int h )
{
    if (_pWater != NULL)
    {
        LOG("create water failture, duplicate create.");
        return false;
    }

    _pWater = new Water(200, 160);

    _pDyncTexture = new atgTexture();
    if (!_pDyncTexture->Create(_pWater->Width, _pWater->Height, TF_R32F, NULL))
    {
        LOG("create TF_R32F texture failture.");
        return false;
    }
    _pDyncTexture->SetFilterMode(TFM_FILTER_NOT_MIPMAP_ONLY_LINEAR);

    _pCamera = new atgCamera();
    _pCamera->SetPosition(Vec3(0,0, 150));
    _pCamera->SetForward(Vec3Back);

    btn[0] = btn [1] = 0;

    return true;
}

void atgSampleWater::Updata()
{
    if (_pWater)
    {
        _pWater->Updata();

        if (_pDyncTexture->IsLost())
        {
           if(!_pDyncTexture->Create(_pWater->Width, _pWater->Height, TF_R32F, NULL))
           {
               return;
           }
        }

        atgTexture::LockData lockData = _pDyncTexture->Lock();
        if (lockData.pAddr)
        {
            if (lockData.pitch > 0)
            {
                char* pSrc = (char*)_pWater->GetBuff();
                char* pDst = (char*)lockData.pAddr;
                int lineSize = _pWater->Width * sizeof(float);
                for (int i = 0; i < _pWater->Height; ++i)
                {
                    memcpy(pDst, pSrc, lineSize);
                    pDst += lockData.pitch;
                    pSrc += lineSize;
                }
            }
            else
            {
                _pWater->CopyTo(lockData.pAddr);
            }

            _pDyncTexture->Unlock();
        }
    }
}

void atgSampleWater::Drop( float xi, float yi )
{
    if (_pWater)
    {
        _pWater->Drop(xi, yi);
    }
}

void atgSampleWater::Render(atgTexture* pTexture /* = 0 */)
{
    const float textureQuadData[] = {
        -100.0f,  100.0f, 20.0f, 0.0f, 0.0f,
         100.0f,  100.0f, 20.0f, 1.0f, 0.0f,
        -100.0f, -100.0f, 20.0f, 0.0f, 1.0f,
         100.0f, -100.0f, 20.0f, 1.0f, 1.0f,
    };

    atgRippleShader* pRippleShader = static_cast<atgRippleShader*>(atgShaderLibFactory::FindOrCreatePass(RIPPLE_PASS_IDENTITY));
    if (NULL == pRippleShader)
    {
        LOG("can't find pass[%s]\n", RIPPLE_PASS_IDENTITY);
        return;
    }

    pRippleShader->SetDxDy(1.0f / float(_pWater->Width - 1), 1.0f / float(_pWater->Height - 1));
    pRippleShader->SetRippleTex(_pDyncTexture);

    Updata();

    if (_pCamera)
    {
        g_Renderer->SetMatrix(MD_VIEW, _pCamera->GetView());
        g_Renderer->SetMatrix(MD_PROJECTION, _pCamera->GetProj()); 
    }

    if (pTexture)
    {
        pRippleShader->SetPrimitiveTex(pTexture);
    }

    g_Renderer->DrawQuadByPass(&textureQuadData[0], &textureQuadData[5], &textureQuadData[10], &textureQuadData[15], &textureQuadData[3], &textureQuadData[8], &textureQuadData[13], &textureQuadData[18], RIPPLE_PASS_IDENTITY);
}

void atgSampleWater::OnKeyScanDown( Key::Scan keyscan )
{
    float moveSpeed = 10.5f;

    switch (keyscan)
    {
    case Key::W:
        {
            Vec3 forward = _pCamera->GetForward();
            Vec3 pos = _pCamera->GetPosition();
            forward.Scale(moveSpeed);
            pos.Add(forward.m);
            _pCamera->SetPosition(pos.m);

        }
        break;
    case Key::S:
        {
            Vec3 forward = _pCamera->GetForward();
            Vec3 pos = _pCamera->GetPosition();
            forward.Scale(-moveSpeed);
            pos.Add(forward.m);
            _pCamera->SetPosition(pos.m);
        }
        break;
    case Key::A:
        {
            Vec3 right = _pCamera->GetRight();
            Vec3 pos = _pCamera->GetPosition();
            right.Scale(moveSpeed);
            pos.Add(right.m);
            _pCamera->SetPosition(pos.m);
        }
        break;
    case Key::D:
        {
            Vec3 right = _pCamera->GetRight();
            Vec3 pos = _pCamera->GetPosition();
            right.Scale(-moveSpeed);
            pos.Add(right.m);
            _pCamera->SetPosition(pos.m);
        }
        break;
    default:
        break;
    }
}

void atgSampleWater::OnPointerDown( uint8 id, int16 x, int16 y )
{
    if(id == 1)
    {
        // right button down
        btn[1] = true;
    }

    if (id == 0)
    {
        btn[0] = true;
        
        uint32 vp[4];
        g_Renderer->GetViewPort(vp[0],vp[1],vp[2],vp[3]);
        Drop(float(x*1.0f/vp[2]), float(y*1.0f/vp[3]));
    }
}

void atgSampleWater::OnPointerMove( uint8 id, int16 x, int16 y )
{
    if(btn[0])
    {
        uint32 vp[4];
        g_Renderer->GetViewPort(vp[0],vp[1],vp[2],vp[3]);
        Drop(float(x*1.0f/vp[2]), float(y*1.0f/vp[3]));
    }

    if (btn[1])
    {

    }

    if (id == MBID_MIDDLE && _pCamera)
    {
        float moveSpeed = 10.5f;
        if (x > 0)
        {
            Vec3 forward = _pCamera->GetForward();
            Vec3 pos = _pCamera->GetPosition();
            atgMath::VecScale(forward.m, moveSpeed, forward.m);
            atgMath::VecAdd(pos.m, forward.m, pos.m);
            _pCamera->SetPosition(pos.m);
        }
        else
        {
            Vec3 forward = _pCamera->GetForward();
            Vec3 pos = _pCamera->GetPosition();
            atgMath::VecScale(forward.m, -moveSpeed, forward.m);
            atgMath::VecAdd(pos.m, forward.m, pos.m);
            _pCamera->SetPosition(pos.m);
        }
    }
}

void atgSampleWater::OnPointerUp( uint8 id, int16 x, int16 y )
{
    if(id == 1)
    {
        // right button down
        btn[1] = false;
    }

    if (id == 0)
    {
        btn[0] = false;
    }
}

atgSampleRenderToTextrue::atgSampleRenderToTextrue()
{
    pDepthTex = 0;
    pColorTex = 0;
    pRT = 0;
}

atgSampleRenderToTextrue::~atgSampleRenderToTextrue()
{
    SAFE_DELETE(pDepthTex);
    SAFE_DELETE(pColorTex);
    SAFE_DELETE(pRT);
}

bool atgSampleRenderToTextrue::Create()
{
    //>�����������
    pDepthTex = new atgTexture();
    if (!pDepthTex->Create(512,512, TF_D24S8, NULL, true))
    {
        return false;
    }

    //>������ɫ����
    pColorTex = new atgTexture();
    if (!pColorTex->Create(512,512, TF_R8G8B8A8, NULL, true))
    {
        return false;
    }

    //>������ɫ��������ģʽ
    pColorTex->SetFilterMode(TFM_FILTER_NOT_MIPMAP_ONLY_LINEAR);
    pColorTex->SetAddressMode(TC_COORD_U, TAM_ADDRESS_CLAMP);
    pColorTex->SetAddressMode(TC_COORD_V, TAM_ADDRESS_CLAMP);

    pRT = new atgRenderTarget();
    std::vector<atgTexture*> colorBuffer; colorBuffer.push_back(pColorTex);
    if (!pRT->Create(colorBuffer, pDepthTex))
    {
        return false;
    }

    return true;
}

void atgSampleRenderToTextrue::OnDrawBegin()
{
    if (pColorTex->IsLost())
    {
        pColorTex->Create(512,512, TF_R8G8B8A8, NULL, true);
    }

    if (pDepthTex->IsLost())
    {
        pDepthTex->Create(512,512, TF_D24S8, NULL, true);
    }

    g_Renderer->PushRenderTarget(0, pRT);

    g_Renderer->Clear();
}

void atgSampleRenderToTextrue::OnDrawEnd()
{
    g_Renderer->PopRenderTarget(0);

    g_Renderer->DrawFullScreenQuad(pColorTex, IsOpenGLGraph());
}

atgFlyCamera::atgFlyCamera():_pCamera(0)
{
    button[0] = button[1] = false;
}

atgFlyCamera::~atgFlyCamera()
{

}

bool atgFlyCamera::Create( class atgCamera* pCamera )
{
    _pCamera = pCamera;

    return true;
}

void atgFlyCamera::OnKeyScanDown( Key::Scan keyscan )
{
    float moveSpeed = 10.5f;

    switch (keyscan)
    {
    case Key::W:
        {
            Vec3 forward = _pCamera->GetForward();
            Vec3 pos = _pCamera->GetPosition();
            forward.Scale(moveSpeed);
            pos.Add(forward.m);
            _pCamera->SetPosition(pos.m);

        }
        break;
    case Key::S:
        {
            Vec3 forward = _pCamera->GetForward();
            Vec3 pos = _pCamera->GetPosition();
            forward.Scale(-moveSpeed);
            pos.Add(forward.m);
            _pCamera->SetPosition(pos.m);
        }
        break;
    case Key::A:
        {
            Vec3 right = _pCamera->GetRight();
            Vec3 pos = _pCamera->GetPosition();
            right.Scale(moveSpeed);
            pos.Add(right.m);
            _pCamera->SetPosition(pos.m);
        }
        break;
    case Key::D:
        {
            Vec3 right = _pCamera->GetRight();
            Vec3 pos = _pCamera->GetPosition();
            right.Scale(-moveSpeed);
            pos.Add(right.m);
            _pCamera->SetPosition(pos.m);
        }
        break;
    default:
        break;
    }
}

void atgFlyCamera::OnPointerDown( uint8 id, int16 x, int16 y )
{
    if(id == 0)
    {
        button[0] = true;
    }

    if(id == 1)
    {
        button[1] = true;
    }
}

void atgFlyCamera::OnPointerMove( uint8 id, int16 x, int16 y )
{
    static int16 last_x = x;
    static int16 last_y = y;

    if (id == MBID_MIDDLE && _pCamera)
    {
        float moveSpeed = 10.5f;
        if (x > 0)
        {
            Vec3 forward = _pCamera->GetForward();
            Vec3 pos = _pCamera->GetPosition();
            forward.Scale(moveSpeed);
            pos.Add(forward.m);
            _pCamera->SetPosition(pos.m);
        }
        else
        {
            Vec3 forward = _pCamera->GetForward();
            Vec3 pos = _pCamera->GetPosition();
            forward.Scale(-moveSpeed);
            pos.Add(forward.m);
            _pCamera->SetPosition(pos.m);
        }
    }
    else
    {
        if (button[1])
        {
            if (_pCamera)
            {
                float oYaw = _pCamera->GetYaw();
                float oPitch = _pCamera->GetPitch();
                float dx = static_cast<float>(x - last_x);
                float dy = static_cast<float>(y - last_y);
                oYaw += dx * 0.001f;
                oPitch += dy * 0.001f;
                oPitch = atgMath::Clamp(oPitch, atgMath::DegreesToRadians(-270.0f), atgMath::DegreesToRadians(-90.0f));
                _pCamera->SetYaw(oYaw);
                _pCamera->SetPitch(oPitch);
            }
        }

        last_x = x;
        last_y = y;
    }
}

void atgFlyCamera::OnPointerUp( uint8 id, int16 x, int16 y )
{
    if(id == 0)
    {
        button[0] = false;
    }

    if(id == 1)
    {
        button[1] = false;
    }
}



//////////////////////////////////////////////////////////////////////////
//>�������
class atgShaderRTSceenDepthColor : public atgShaderLibPass
{
public:
    atgShaderRTSceenDepthColor();
    ~atgShaderRTSceenDepthColor();

    virtual bool			ConfingAndCreate();

    void                    SetMatirxOfLightViewPojection(const Matrix& mat) { _ligthViewProj = mat; }

protected:
    virtual void			BeginContext(void* data);
    
    Matrix                  _ligthViewProj;
};

#define RT_DEPTH_COLOR_PASS_IDENTITY "atgSceenDepthColorShader"
EXPOSE_INTERFACE(atgShaderRTSceenDepthColor, atgPass, RT_DEPTH_COLOR_PASS_IDENTITY);

atgShaderRTSceenDepthColor::atgShaderRTSceenDepthColor()
{

}

atgShaderRTSceenDepthColor::~atgShaderRTSceenDepthColor()
{

}

bool atgShaderRTSceenDepthColor::ConfingAndCreate()
{
    bool rs = false;

    if (IsOpenGLGraph())
    {
        rs = atgPass::Create("shaders/rt_depth_color.glvs", "shaders/rt_depth_color.glfs");
    }
    else
    {
        rs = atgPass::Create("shaders/rt_depth_color.dxvs", "shaders/rt_depth_color.dxps");
    }

    return rs;
}

void atgShaderRTSceenDepthColor::BeginContext(void* data)
{
    atgPass::BeginContext(data);

    // set matrix.
    SetMatrix4x4("mat_light_view_Projection", _ligthViewProj);
}


//////////////////////////////////////////////////////////////////////////
//>�������
class atgShaderShadowMapping : public atgShaderLibPass
{
public:
    atgShaderShadowMapping();
    ~atgShaderShadowMapping();

    virtual bool			ConfingAndCreate();


    void                    SetMatirxOfLightViewPojection(const Matrix& mat) { _ligthViewProj = mat; }
    void                    SetColorDepthTex(atgTexture* pTex) { _pColorDepthTex = pTex; }

    void                    SetLight(const Vec3& ligPos, const Vec3& ligDir) { _ligPos = ligPos; _ligDir = ligDir; }
    void                    SetSpotParam(float outerCone, float innerCone) { _spot_outer_cone = outerCone;  _spot_inner_cone = innerCone; }
    void                    SetAmbientColor(const Vec4& color) { _ambientColor = color; }
    void                    SetBias(float bias) { _bias = bias; }

protected:
    virtual void			BeginContext(void* data);

    Matrix                  _ligthViewProj;
    atgTexture*             _pColorDepthTex;

    Vec3                    _ligPos;
    Vec3                    _ligDir;
    float                   _spot_outer_cone;
    float                   _spot_inner_cone;
    Vec4                    _ambientColor;
    float                   _bias;
};

#define SHADOW_MAPPING_PASS_IDENTITY "atgShadowMappingShader"
EXPOSE_INTERFACE(atgShaderShadowMapping, atgPass, SHADOW_MAPPING_PASS_IDENTITY);


atgShaderShadowMapping::atgShaderShadowMapping():_pColorDepthTex(0)
{

}

atgShaderShadowMapping::~atgShaderShadowMapping()
{

}

bool atgShaderShadowMapping::ConfingAndCreate()
{
    bool rs = false;

    if (IsOpenGLGraph())
    {
        rs = atgPass::Create("shaders/rt_shadow_mapping.glvs", "shaders/rt_shadow_mapping.glfs");
    }
    else
    {
        rs = atgPass::Create("shaders/rt_shadow_mapping.dxvs", "shaders/rt_shadow_mapping.dxps");
    }

    return rs;
}

void atgShaderShadowMapping::BeginContext(void* data)
{
    atgPass::BeginContext(data);

    // set matrix.
    SetMatrix4x4("mat_light_view_Projection", _ligthViewProj);

    // set texture
    g_Renderer->BindTexture(0, _pColorDepthTex);
    SetTexture("rtDepthSampler", 0);

    // set light
    SetFloat3("LightPosition", _ligPos.m);
    SetFloat3("LightDirection", _ligDir.m);
    SetFloat("spot_outer_cone", _spot_outer_cone);
    SetFloat("spot_inner_cone", _spot_inner_cone);

    // set bias
    SetFloat("bias", _bias);

    // set ambient
    SetFloat4("ambient", _ambientColor.m);

    // set port size
    uint32 viewPort[4];
    g_Renderer->GetViewPort(viewPort[0], viewPort[1], viewPort[2], viewPort[3]);
    float viewPortF[2];
    viewPortF[0] = (float)viewPort[2];
    viewPortF[1] = (float)viewPort[3];
    SetFloat2("fViewportDimensions", viewPortF);
}

//////////////////////////////////////////////////////////////////////////
//>Ӱ����ͼ
atgSimpleShadowMapping::atgSimpleShadowMapping():pRT(0),pPixelDepthTex(0),pNormalDepthTex(0)
{
    bias = 0.00005f;
    d_far = 1000.f;
    d_near = 0.1f;
}

atgSimpleShadowMapping::~atgSimpleShadowMapping()
{
    SAFE_DELETE(pPixelDepthTex);
    SAFE_DELETE(pNormalDepthTex);
    SAFE_DELETE(pRT);
}

bool atgSimpleShadowMapping::Create()
{
    const int textureSize = 256;
    pPixelDepthTex = new atgTexture();
    if (false == pPixelDepthTex->Create(textureSize, textureSize, IsOpenGLGraph() ? TF_R8G8B8A8 : TF_R32F, NULL, true))
    {
        return false;
    }
    pPixelDepthTex->SetFilterMode(TFM_FILTER_NOT_MIPMAP_ONLY_LINEAR);
    
    pNormalDepthTex = new atgTexture();
    if (false == pNormalDepthTex->Create(textureSize, textureSize, TF_D16, NULL, true))
    {
        return false;
    }

    std::vector<atgTexture*> colorBuffer;
    colorBuffer.push_back(pPixelDepthTex);
    pRT = new atgRenderTarget();
    if (false == pRT->Create(colorBuffer, pNormalDepthTex))
    {
        return false;
    }

    lightPos.Set(150.f*1.2f,180.f*1.2f,-110.f*1.2f);
    lightDir.Set(-0.65f,-0.73f,0.172f);

    //lightPos.Set(1149.f*0.5f,2113.f*0.5f,-324.f*0.5f);
    //lightDir.Set(-0.326f,-0.93f,0.166f);

    lightDir.Normalize();
    atgMath::LookAt(lightPos.m, (lightPos + lightDir).m, Vec3Up.m, lightViewMatrix.m);

    return true;
}

void atgSimpleShadowMapping::Render(class atgCamera* sceneCamera)
{
    DrawDepthTex(sceneCamera);
    DrawSceen(sceneCamera);
}

void atgSimpleShadowMapping::OnKeyScanDown( Key::Scan keyscan )
{
    switch (keyscan)
    {
    case Key::Semicolon:
        {
            bias += 0.0001f;
            LOG("new bias[%f]\n", bias);
            break;
        }
    case Key::Apostrophe:
        {
            bias -= 0.0001f;
            LOG("new bias[%f]\n", bias);
            break;
        }
    case Key::Comma: //<
        {
            d_far += 10.f;
            LOG("new d_far[%f]\n", d_far);
        }break;
    case Key::Period: //>
        {
            d_far -= 10.f;
            LOG("new d_far[%f]\n", d_far);

        }break;
    case Key::LeftBracket: //[
        {
            //bias += 0.000001f;
            //LOG("new bias[%f]\n", bias);
            d_near += 10.f;
            LOG("new d_near[%f]\n", d_near);
        }break;
    case Key::RightBracket: //]
        {
            d_near -= 10.f;
            LOG("new d_near[%f]\n", d_near);
            //bias -= 0.000001f;
            //LOG("new bias[%f]\n", bias);
        }break;
    default:
        break;
    }
}

void atgSimpleShadowMapping::OnPointerMove( uint8 id, int16 x, int16 y )
{
#ifndef _WIN32
    if (id == 1)
    {
        bias -= 0.000001f;
    }else
    {
        bias += 0.000001f;
    }

    LOG("new bias[%f]\n", bias);
#endif // !_WIN32
}

void atgSimpleShadowMapping::DrawDepthTex(class atgCamera* sceneCamera)
{
    if (pRT)
    {
        g_Renderer->PushRenderTarget(0, pRT);
        
        g_Renderer->Clear();

        atgPass* pPass = atgShaderLibFactory::FindOrCreatePass(RT_DEPTH_COLOR_PASS_IDENTITY);
        if (pPass == NULL)
        {
            LOG("can't find pass[%s]\n", RT_DEPTH_COLOR_PASS_IDENTITY);
            return;
        }

        //>fov��60������Ӱ��Ч���ȽϺ�.
        Matrix projMat;
        atgMath::Perspective(60.0f, 1.0f, d_near, d_far, projMat.m);
        ((atgShaderRTSceenDepthColor*)pPass)->SetMatirxOfLightViewPojection(projMat.Concatenate(lightViewMatrix));
        
        DrawBox(RT_DEPTH_COLOR_PASS_IDENTITY);
        DrawPlane(RT_DEPTH_COLOR_PASS_IDENTITY);
        
        g_Renderer->PopRenderTarget(0);
    }
}

void atgSimpleShadowMapping::DrawSceen(class atgCamera* sceneCamera)
{
    g_Renderer->SetMatrix(MD_VIEW, sceneCamera->GetView());
    g_Renderer->SetMatrix(MD_PROJECTION, sceneCamera->GetProj());

    atgPass* pPass = atgShaderLibFactory::FindOrCreatePass(SHADOW_MAPPING_PASS_IDENTITY);
    if (pPass == NULL)
    {
        LOG("can't find pass[%s]\n", SHADOW_MAPPING_PASS_IDENTITY);
        return;
    }

    atgShaderShadowMapping* pShadowPass = (atgShaderShadowMapping*)pPass;
    
    //>fov��60������Ӱ��Ч���ȽϺ�.
    Matrix projMat;
    atgMath::Perspective(60.0f, 1.0f, d_near, d_far, projMat.m);
    pShadowPass->SetMatirxOfLightViewPojection(projMat.Concatenate(lightViewMatrix));    
    pShadowPass->SetColorDepthTex(pPixelDepthTex);
    //pShadowPass->SetAmbientColor(GetVec4Color(YD_COLOR_NAVAJO_WHITE));
    pShadowPass->SetAmbientColor(Vec4(0.3f, 0.3f, 0.3f, 1.0f));
    pShadowPass->SetLight(lightPos, lightDir);
    pShadowPass->SetSpotParam(30.0f, 15.0f);
    pShadowPass->SetBias(bias);
    
    DrawBox(SHADOW_MAPPING_PASS_IDENTITY);
    DrawPlane(SHADOW_MAPPING_PASS_IDENTITY);

    uint32 oldVP[4];
    g_Renderer->GetViewPort(oldVP[0], oldVP[1], oldVP[2], oldVP[3]);
    uint32 newVP[4] = {0, IsOpenGLGraph() ? 0 : oldVP[3] - 200, 200, 200 };
    g_Renderer->SetViewPort(newVP[0], newVP[1], newVP[2], newVP[3]);

    g_Renderer->DrawFullScreenQuad(pPixelDepthTex, IsOpenGLGraph());

    g_Renderer->SetViewPort(oldVP[0], oldVP[1], oldVP[2], oldVP[3]);
    
}

void atgSimpleShadowMapping::DrawBox(const char* pPassIdentity /* = NULL */)
{
    g_Renderer->BeginFullQuad();

    Vec3 startPos;
    float size = 100.0f;

    Vec3 color = GetVec3Color(YD_COLOR_DARK_TURQUOISE).m;

    //>ǰ
    g_Renderer->AddFullQuad( startPos.m,                            // 1
                            (startPos + Vec3(size, 0, 0)).m,        // 2
                            (startPos + Vec3(0, -size, 0)).m,       // 3
                            (startPos + Vec3(size, -size, 0)).m,    // 4
                             color.m); 

    //>��
    g_Renderer->AddFullQuad((startPos + Vec3(0, 0, -size)).m,       // 5
                            (startPos + Vec3(0, -size, -size)).m,   // 6
                            (startPos + Vec3(size, 0, -size)).m,    // 7
                            (startPos + Vec3(size, -size, -size)).m,// 8
                             color.m);  

    //>��
    g_Renderer->AddFullQuad((startPos + Vec3(0, 0, -size)).m,       // 5
                             startPos.m,                            // 1
                            (startPos + Vec3(0, -size, -size)).m,   // 6
                            (startPos + Vec3(0, -size, 0)).m,       // 3
                             color.m);

    //>��
    g_Renderer->AddFullQuad((startPos + Vec3(size, 0, 0)).m,        // 2
                            (startPos + Vec3(size, 0, -size)).m,    // 7
                            (startPos + Vec3(size, -size, 0)).m,    // 4
                            (startPos + Vec3(size, -size, -size)).m,// 8
                             color.m);

    //>��
    g_Renderer->AddFullQuad((startPos + Vec3(0, 0, -size)).m,       // 5
                            (startPos + Vec3(size, 0, -size)).m,    // 7
                             startPos.m,                            // 1
                            (startPos + Vec3(size, 0, 0)).m,        // 2
                            color.m);


    //>��
    g_Renderer->AddFullQuad((startPos + Vec3(0, -size, 0)).m,       // 3
                            (startPos + Vec3(size, -size, 0)).m,    // 4
                            (startPos + Vec3(0, -size, -size)).m,   // 6
                            (startPos + Vec3(size, -size, -size)).m,// 8
                             color.m);



    g_Renderer->EndFullQuad(pPassIdentity);
}

void atgSimpleShadowMapping::DrawPlane(const char* pPassIdentity /* = NULL */)
{
    g_Renderer->BeginFullQuad();

    Vec3 startPos(-250.f, -100.0f, 250.0f);
    float size = 500.0f;

    Vec3 color = GetVec3Color(YD_COLOR_PERU).m;

    g_Renderer->AddFullQuad( startPos.m,                            // 1
                            (startPos + Vec3(0, 0, -size)).m,       // 2
                            (startPos + Vec3(size, 0, 0)).m,        // 3
                            (startPos + Vec3(size, 0, -size)).m,    // 4
                             color.m);

    g_Renderer->EndFullQuad(pPassIdentity);
}

