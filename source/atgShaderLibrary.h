#pragma once

#include "atgMisc.h"
#include "atgRenderer.h"

class atgMaterial;

class atgShaderLibPass : public atgPass
{
public:
    virtual bool            ConfingAndCreate() = 0;
};

class atgShaderLibFactory
{
public:
    static atgPass*         FindOrCreatePass(const char* Identity, int *pReturnCode = NULL);
};
// atgShaderLibFactory::FindOrCreatePass pReturnCode Value 
extern const int IPASS_CONFIG_FAILD;  // -2

//////////////////////////////////////////////////////////////////////////
class atgShaderSolidColor : public atgShaderLibPass
{
public:
    atgShaderSolidColor();
    ~atgShaderSolidColor();

    virtual bool            ConfingAndCreate();

    void                    SetSolidColor(const Vec3& SolidColor) { m_SolidColor = SolidColor; }
protected:
    virtual void            BeginContext(void* data);
protected:
    Vec3                    m_SolidColor;
};

#define SOLIDCOLOR_PASS_IDENTITY "SolidColorShader"
EXPOSE_INTERFACE(atgShaderSolidColor, atgPass, SOLIDCOLOR_PASS_IDENTITY);

//////////////////////////////////////////////////////////////////////////
class atgShaderNotLighteTexture : public atgShaderLibPass
{
public:
    atgShaderNotLighteTexture();
    ~atgShaderNotLighteTexture();

    virtual bool            ConfingAndCreate();
protected:
    virtual void            BeginContext(void* data);

    void                    SetTexture(atgTexture* texture);
protected:
    atgTexture*             m_Texture;
};

#define NOT_LIGNTE_TEXTURE_PASS_IDENTITY "NotLightTextureShader"
EXPOSE_INTERFACE(atgShaderNotLighteTexture, atgPass, NOT_LIGNTE_TEXTURE_PASS_IDENTITY);


//////////////////////////////////////////////////////////////////////////
class atgShaderLightTexture : public atgShaderNotLighteTexture
{
public:
    atgShaderLightTexture();
    ~atgShaderLightTexture();

    virtual bool            ConfingAndCreate();
protected:
    virtual void            BeginContext(void* data);
};

#define LIGHT_TEXTURE_PASS_IDENTITY "LightTextureShader"
EXPOSE_INTERFACE(atgShaderLightTexture, atgPass, LIGHT_TEXTURE_PASS_IDENTITY);
