#include "atgBase.h"
#include "atgRenderer.h"
#include "atgGame.h"
#include "atgMisc.h"
#include "atgProfile.h"

#ifdef USE_OPENGL

inline bool IsSpace(char character)
{
    if (character == ' ' || character == '\t' || character == '\r')
    {
        return true;
    }
    return false;
}

bool ReadShadeFile(GLchar** shadeCode, const char* shadeFile)
{
    atgReadFile reader;
    long fileSize;
    GLchar lineBuffer[512];
    GLchar* lineBufferPtr = lineBuffer;
    GLchar* destCode;
    if(reader.Open(shadeFile))
    {
        fileSize = reader.GetLength();
        if (fileSize <= 0)
            return false;
        (*shadeCode) = new GLchar[fileSize + 1];
        reader.Read((*shadeCode), fileSize);
        (*shadeCode)[fileSize] = '\0';
        destCode = (*shadeCode);

        std::string tempShaderCode;
        std::string Line = "";
        int codeIndex = -1;
        bool fileEnd = false;
        while(!fileEnd)
        {
            Line.clear();
            for(++codeIndex;destCode[codeIndex] != '\n'; ++codeIndex)
            {
                Line += destCode[codeIndex];
                if (codeIndex >= fileSize)
                {
                    fileEnd = true;
                    break;
                }
            }

            size_t length = Line.length();
            size_t index;
            for (index = 0; index < length && IsSpace(Line[index]); index++)
                ;
            if (index >= length)
                continue;
            if (Line[index] == '/' && ((index + 1) < length) && Line[index + 1] == '/')
                continue;
            tempShaderCode += Line.substr(index) + '\n';
        }
        size_t codeSize = tempShaderCode.size();
        *shadeCode = new GLchar[codeSize + 1];
        memcpy(*shadeCode, tempShaderCode.c_str(), codeSize);
        (*shadeCode)[codeSize] = '\0';
        reader.Close();
        LOG("%s\n", *shadeCode);
        return true;
    }
    //std::string tempShaderCode;
    //std::ifstream VertexShaderStream (shadeFile, std::ios::in);
    //if(VertexShaderStream.is_open())
    //{
    //    std::string Line = "";
    //    while(getline(VertexShaderStream, Line))
    //    {
    //        size_t length = Line.length();
    //        size_t index;
    //        for (index = 0; index < length && IsSpace(Line[index]); index++)
    //            ;
    //        if (index >= length)
    //          continue;
    //        if (Line[index] == '/' && ((index + 1) < length) && Line[index + 1] == '/')
    //            continue;
    //        tempShaderCode += '\n' + Line.substr(index);
    //    }
    //    VertexShaderStream.close();
    //    size_t codeSize = tempShaderCode.size();
    //    *shadeCode = new GLchar[codeSize + 1];
    //    memcpy(*shadeCode, tempShaderCode.c_str(), codeSize);
    //    (*shadeCode)[codeSize] = '\0';
    //    return true;
    //}
    return false;
}

//////////////////////////////////////////////////////////////////////////
// index buffer implement
class atgIndexBufferImpl
{
public:
    atgIndexBufferImpl():vbo_indicesID(0),accessMode(BAM_Static),locked(false),lockOffset(0),lockSize(0),pLockMemory(0) {}
    ~atgIndexBufferImpl() 
    {
        SAFE_DELETE_ARRAY(pLockMemory); 
        if(vbo_indicesID)
        {
            GL_ASSERT( glDeleteBuffers(1, &vbo_indicesID) );
            vbo_indicesID = 0;
        }
    }

    bool Bind() 
    {
        GL_ASSERT( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_indicesID) );
        return true;
    }
    
    void Unbind()
    {
        GL_ASSERT( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0) );
    }
public:
    GLuint vbo_indicesID;
    BufferAccessMode accessMode;
    bool locked;
    GLsizei lockOffset;
    GLuint lockSize;
    char* pLockMemory;
};

atgIndexBuffer::atgIndexBuffer():_impl(NULL)
{
}

atgIndexBuffer::~atgIndexBuffer()
{
    Destory();
    g_Renderer->RemoveGpuResource(this);
}

bool atgIndexBuffer::Create( uint16 *pIndices, uint32 numIndices, IndexFormat format, BufferAccessMode accessMode )
{
    Destory();

    int indexSize = sizeof(uint16);
    int bufSize = numIndices * indexSize;
    _impl = new atgIndexBufferImpl;
    _impl->accessMode = accessMode;

    GL_ASSERT( glGenBuffers(1, &(_impl->vbo_indicesID)) );
    _impl->Bind();
    GL_ASSERT( glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufSize, pIndices, (_impl->accessMode == BAM_Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW ) );
    _size = bufSize;
    _impl->pLockMemory = new char[_size];

    _impl->Unbind();

    atgGpuResource::ReSet();
    g_Renderer->InsertGpuResource(this, static_cast<GpuResDestoryFunc>(&atgIndexBuffer::Destory));
    //LOG("create a atgIndexBuffer[%p].\n", this);

    return true;
}

bool atgIndexBuffer::Destory()
{
    atgGpuResource::Lost();
    SAFE_DELETE(_impl);

    return true;
}

void* atgIndexBuffer::Lock( uint32 offset, uint32 size )
{
    AASSERT(_impl);
    void* pIndexBuffer = NULL;
    if (!IsLost() && _impl && size <= _size)
    {
        _impl->lockOffset = offset;
        _impl->lockSize = size;
        pIndexBuffer = _impl->pLockMemory;
        _impl->locked = true;
    }

    return pIndexBuffer;
}

void atgIndexBuffer::Unlock()
{
    if(_impl->locked && _impl->lockSize > 0)
    {
        _impl->Bind();
        if (_impl->lockSize == _size)
        {
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, _size, _impl->pLockMemory, (_impl->accessMode == BAM_Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, _impl->lockOffset, _impl->lockSize, _impl->pLockMemory);
        }
        _impl->Unbind();
        _impl->locked = false;
    }
}

bool atgIndexBuffer::IsLocked() const
{
    if(_impl)
        return _impl->locked;

    return false;
}

bool atgIndexBuffer::Bind()
{
    if (_impl)
    {
        return _impl->Bind();
    }
    return false;
}

void atgIndexBuffer::Unbind()
{
    if (_impl)
    {
        _impl->Unbind();
    }
}

//////////////////////////////////////////////////////////////////////////
// Vertex Declaration implement
class atgVertexDeclImpl
{
public:
    atgVertexDeclImpl():numberElements(0),stride(0) {}
    atgVertexDeclImpl(const atgVertexDeclImpl& other)
    {
        numberElements = other.numberElements;
        stride = other.stride;
        for (uint8 i = 0; i < numberElements; ++i)
        {
            element[i].streamIndex = other.element[i].streamIndex;
            element[i].attribute = other.element[i].attribute;
        }
    }
public:
    uint8 numberElements;
    uint8 stride;
    typedef struct DeclarationElement_t
    {
        uint32 streamIndex;
        atgVertexDecl::VertexAttribute attribute;
        DeclarationElement_t():streamIndex(0),attribute(atgVertexDecl::VA_Max){}
    }DeclarationElement;
    DeclarationElement element[atgVertexDecl::MaxNumberElement];
};

atgVertexDecl::atgVertexDecl()
{
    _impl = new atgVertexDeclImpl();
}

atgVertexDecl::atgVertexDecl(const atgVertexDecl& other):_impl(NULL)
{
    *this = other;
}

atgVertexDecl& atgVertexDecl::operator= (const atgVertexDecl& other)
{
    SAFE_DELETE(_impl);
    _impl = new atgVertexDeclImpl();
    *_impl = *other._impl;
    return *this;
}

atgVertexDecl::~atgVertexDecl()
{
    SAFE_DELETE(_impl);
}

uint8 atgVertexDecl::GetNumberElement() const
{
    return _impl->numberElements;
}

uint8 atgVertexDecl::GetElementsStride() const
{
    return _impl->stride;
}

bool atgVertexDecl::AppendElement( uint32 streamIndex, atgVertexDecl::VertexAttribute attribute )
{
    int numberElements = _impl->numberElements;
    if ((numberElements + 1) >= MaxNumberElement)
    {
        LOG("%s numberElements >= %d", __FUNCTION__, MaxNumberElement);
        return false;
    }

    _impl->element[numberElements].streamIndex = streamIndex;
    _impl->element[numberElements].attribute = attribute;
    switch(attribute)
    {
    case atgVertexDecl::VA_Pos3:
    case atgVertexDecl::VA_Normal:
    case atgVertexDecl::VA_Diffuse:
        {
            _impl->stride += 12;
        }break;
    case atgVertexDecl::VA_Pos4:
    case atgVertexDecl::VA_Tangent:
    case atgVertexDecl::VA_BlendFactor4:
        {
            _impl->stride += 16;
        }break;
    case atgVertexDecl::VA_Specular:
        {
            _impl->stride += 12;
        }break;
    case atgVertexDecl::VA_Texture0:
    case atgVertexDecl::VA_Texture1:
    case atgVertexDecl::VA_Texture2:
    case atgVertexDecl::VA_Texture3:
    case atgVertexDecl::VA_Texture4:
    case atgVertexDecl::VA_Texture5:
    case atgVertexDecl::VA_Texture6:
    case atgVertexDecl::VA_Texture7:
        {
            _impl->stride += 8;
        }break;
    default:
        {
            AASSERT(0 && "error declararion type.\n");
        }break;
    }
    ++(_impl->numberElements);
    return true;
}

atgVertexDecl::VertexAttribute atgVertexDecl::GetIndexElement(uint32 streamIndex, uint32 index) const
{
    return _impl->element[index].attribute;
}

//////////////////////////////////////////////////////////////////////////
// vertex buffer
class atgVertexBufferImpl
{
public:
    atgVertexBufferImpl():vbo_vertexbufferID(0),accessMode(BAM_Static),locked(false),lockOffset(0),lockSize(0),pLockMemory(0) {}
    ~atgVertexBufferImpl()
    {
        SAFE_DELETE_ARRAY(pLockMemory); 
        if(vbo_vertexbufferID) 
        { 
            GL_ASSERT( glDeleteBuffers(1, &vbo_vertexbufferID) ); 
            vbo_vertexbufferID = 0; 
        }
    }
    bool Bind();
    void Unbind();
public:
    atgVertexDecl decl;
    GLuint vbo_vertexbufferID;
    BufferAccessMode accessMode;
    bool locked;
    GLsizei lockOffset;
    GLuint lockSize;
    char* pLockMemory;
};

bool atgVertexBufferImpl::Bind()
{
    GLint currentProgramID;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgramID);
    if (currentProgramID < 0){
        AASSERT(0);
        return false;
    }

    GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, vbo_vertexbufferID) );
    GLint index = -1;
    GLint size = 0;
    GLenum type = GL_FLOAT;
    GLboolean normalized = GL_FALSE;
    GLuint offset = 0;
    uint32 numberElement = decl.GetNumberElement();
    GLuint stride = decl.GetElementsStride();
    for (uint32 i = 0; i < numberElement; ++i)
    {
        atgVertexDecl::VertexAttribute attribute = decl.GetIndexElement(0, i);
        switch(attribute)
        {
        case atgVertexDecl::VA_Pos3:
            {
                GL_ASSERT( index = glGetAttribLocation(currentProgramID, OGL_V_VP) );
                size = 3;
                break;
            }
        case atgVertexDecl::VA_Normal:
            {
                GL_ASSERT( index = glGetAttribLocation(currentProgramID, OGL_V_VN) );
                size = 3;
                break;
            }
        case atgVertexDecl::VA_Tangent:
            {
                GL_ASSERT( index = glGetAttribLocation(currentProgramID, OGL_V_VT) );
                size = 4;
                break;
            }
        case atgVertexDecl::VA_Pos4:
            {
                GL_ASSERT( index = glGetAttribLocation(currentProgramID, OGL_V_VP) );
                size = 4;
                break;
            }
        case atgVertexDecl::VA_Diffuse:
            {
                GL_ASSERT( index = glGetAttribLocation(currentProgramID, OGL_V_VDC) );
                size = 3;
                break;
            }
        case atgVertexDecl::VA_Specular:
            {
                GL_ASSERT( index = glGetAttribLocation(currentProgramID, OGL_V_VSC) );
                size = 4;
                break;
            }
        case atgVertexDecl::VA_Texture0:
            {
                GL_ASSERT( index = glGetAttribLocation(currentProgramID, OGL_V_VT0) );
                size = 2;
                break;
            }
        case atgVertexDecl::VA_Texture1:
            {
                GL_ASSERT( index = glGetAttribLocation(currentProgramID, OGL_V_VT1) );
                size = 2;
                break;
            }
        case atgVertexDecl::VA_Texture2:
        case atgVertexDecl::VA_Texture3:
        case atgVertexDecl::VA_Texture4:
        case atgVertexDecl::VA_Texture5:
        case atgVertexDecl::VA_Texture6:
        case atgVertexDecl::VA_Texture7:
            {
                size = 2;
                break;
            }
        case atgVertexDecl::VA_PointSize:
        case atgVertexDecl::VA_BlendFactor4:
        default:
            {
                AASSERT(0 && "error declararion type.\n");
                break;
            }
        }
        if(index != -1)
        {
            GL_ASSERT( glEnableVertexAttribArray(index) );
            GL_ASSERT( glVertexAttribPointer( index, size, type, normalized, stride, (const void*)offset) );
        }
        offset += (size * sizeof(float));
    }

    return true;
}

void atgVertexBufferImpl::Unbind()
{
    uint32 numberElement = decl.GetNumberElement();
    for (uint32 i = 0; i < numberElement; ++i)
    {
        GL_ASSERT( glDisableVertexAttribArray( (GLuint)i ) );
    }
    GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, 0) );
}

atgVertexBuffer::atgVertexBuffer():_impl(NULL),_size(0)
{
}

atgVertexBuffer::~atgVertexBuffer()
{
    Destory();
    g_Renderer->RemoveGpuResource(this);
}

bool atgVertexBuffer::Create( atgVertexDecl* decl, const void *pData, uint32 size, BufferAccessMode accessMode )
{
    Destory();

    _impl = new atgVertexBufferImpl();
    _impl->decl = *decl;
    _impl->accessMode = accessMode;

    // generate VBO
    GL_ASSERT( glGenBuffers(1, &(_impl->vbo_vertexbufferID)) );
    GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, _impl->vbo_vertexbufferID) );
    GL_ASSERT( glBufferData(GL_ARRAY_BUFFER, size, pData, (accessMode == BAM_Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW) );
    _size = size;
    _impl->pLockMemory = new char[_size];

    GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, 0));

    atgGpuResource::ReSet();
    g_Renderer->InsertGpuResource(this, static_cast<GpuResDestoryFunc>(&atgVertexBuffer::Destory));
    //LOG("create a atgVertexBuffer[%p].\n", this);

    return true;
}

bool atgVertexBuffer::Destory()
{
    atgGpuResource::Lost();
    SAFE_DELETE(_impl);

    return true;
}

void* atgVertexBuffer::Lock( uint32 offset, uint32 size )
{
    AASSERT(_impl);
    void* pVertexBuffer = NULL;
    if (!IsLost() && _impl && size <= _size)
    {
        _impl->lockOffset = offset;
        _impl->lockSize = size;
        pVertexBuffer = _impl->pLockMemory;
        _impl->locked = true;
    }

    return pVertexBuffer;

//#ifdef OPENGL_USE_MAP
//    if (_impl)
//    {
//        if (!_impl->locked)
//        {
//            GLvoid* buffer = NULL;
//            GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, _impl->vbo_vertexbufferID) );
//            GL_ASSERT( buffer = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY) );
//            _impl->locked = true;
//            return buffer;
//        }
//    }
//#else
//    SAFE_DELETE_ARRAY(_impl->pLockBuffer);
//    _impl->pLockBuffer = new char[size];
//    _impl->lockBufferSize = size;
//    _impl->offsetLockBuffer = offset;
//    _impl->locked = true;
//    return (void*)_impl->pLockBuffer;
//#endif
//    return NULL;

}

void atgVertexBuffer::Unlock()
{
    if(_impl->locked && _impl->lockSize > 0)
    {
        GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, _impl->vbo_vertexbufferID) );
        if (_impl->lockSize == _size)
        {
            glBufferData(GL_ARRAY_BUFFER, _size, _impl->pLockMemory, (_impl->accessMode == BAM_Dynamic) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);
        }
        else
        {
            glBufferSubData(GL_ARRAY_BUFFER, _impl->lockOffset, _impl->lockSize, _impl->pLockMemory);
        }
        GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, 0));
        _impl->locked = false;
    }


//#ifdef OPENGL_USE_MAP
//    if (_impl)
//    {
//        if (_impl->locked)
//        {
//            GL_ASSERT( glUnmapBuffer(GL_ARRAY_BUFFER) );
//            _impl->locked = false;
//        }
//    }
//#else
//    if (_impl)
//    {
//        if (_impl->locked)
//        {
//            if(_impl->pLockBuffer != NULL)
//            {
//                GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, _impl->vbo_vertexbufferID) );
//                GL_ASSERT( glBufferSubData(GL_ARRAY_BUFFER, _impl->offsetLockBuffer, _impl->lockBufferSize, (GLvoid*)_impl->pLockBuffer) );
//                SAFE_DELETE_ARRAY(_impl->pLockBuffer);
//            }
//            _impl->locked = false;
//        }
//    }
//#endif
}

bool atgVertexBuffer::IsLocked() const
{
    if(_impl)
        return _impl->locked;

    return false;
}

bool atgVertexBuffer::Bind()
{
    if (_impl)
        return _impl->Bind();

    return false;
}

void atgVertexBuffer::Unbind()
{
    if (_impl)
    {
        _impl->Unbind();
    }
}

class atgTextureImpl
{
public:
    atgTextureImpl():TextureID(0),internalFormat(0),inFormat(0),type(0),isColorFormat(0),locked(false),isFloatData(0),pixelSize(0),pLockMemory(0),index(0) {}
    ~atgTextureImpl() 
    {
        SAFE_DELETE_ARRAY(pLockMemory);
        if(TextureID)
        {
            if (isColorFormat)
            {
                GL_ASSERT( glDeleteTextures(1, &TextureID) );
            }
            else
            {
                GL_ASSERT( glDeleteRenderbuffers(1, &TextureID) );
            }
            TextureID = 0;
        }
    }

    bool Bind(uint8 index_)
    {
        if (index_ >= atgRenderer::MaxNumberBindTexture)
            return false;

        index = index_;

        // Bind our texture in Texture Unit + index
        GL_ASSERT( glActiveTexture(GL_TEXTURE0 + index) );
        GL_ASSERT( glBindTexture(GL_TEXTURE_2D, TextureID) );

        return true;
    }

    void Unbind(uint8 index)
    {
        if (index >= atgRenderer::MaxNumberBindTexture)
            return;

        glActiveTexture(GL_TEXTURE0 + index);
        GL_ASSERT( glBindTexture(GL_TEXTURE_2D, 0) );
    }
public:
    GLuint TextureID;
    GLint internalFormat;
    GLenum inFormat;
    GLenum type;
    bool isColorFormat;

    bool locked;
    bool isFloatData;
    uint8 pixelSize;
    char* pLockMemory;
    uint8 index;

};

atgTexture::atgTexture():_width(0),_height(0),_format(TF_R8G8B8A8),_impl(NULL)
{
    _filter = MAX_FILTERMODES;
    for (int i = 0; i < MAX_COORDS; ++i)
    {
        _address[i] = MAX_ADDRESSMODES;
    }
}

atgTexture::~atgTexture()
{
    Destory();
    g_Renderer->RemoveGpuResource(this);
}

bool atgTexture::Create( uint32 width, uint32 height, TextureFormat format, const void *pData/*=NULL*/, bool useToRenderTarget/*=false*/ )
{
    /*
    OpenGL ES 2.0

    void glTexImage2D(	GLenum target,
                        GLint level,
                        GLint internalformat,
                        GLsizei width,
                        GLsizei height,
                        GLint border,
                        GLenum format,
                        GLenum type,
                        const GLvoid * data);

    Parameters

    target
        Specifies the target texture of the active texture unit. Must be GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, GL_TEXTURE_CUBE_MAP_POSITIVE_Z, or GL_TEXTURE_CUBE_MAP_NEGATIVE_Z.

    level
        Specifies the level-of-detail number. Level 0 is the base image level. Level n is the nth mipmap reduction image.

    internalformat
        Specifies the internal format of the texture. Must be one of the following symbolic constants: GL_ALPHA, GL_LUMINANCE, GL_LUMINANCE_ALPHA, GL_RGB, GL_RGBA.

    width
        Specifies the width of the texture image. All implementations support 2D texture images that are at least 64 texels wide and cube-mapped texture images that are at least 16 texels wide.

    height
        Specifies the height of the texture image All implementations support 2D texture images that are at least 64 texels high and cube-mapped texture images that are at least 16 texels high.

    border
        Specifies the width of the border. Must be 0.

    format
        Specifies the format of the texel data. Must match internalformat. The following symbolic values are accepted: GL_ALPHA, GL_RGB, GL_RGBA, GL_LUMINANCE, and GL_LUMINANCE_ALPHA.

    type
        Specifies the data type of the texel data. The following symbolic values are accepted: GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_4_4_4_4, and GL_UNSIGNED_SHORT_5_5_5_1.

    data
        Specifies a pointer to the image data in memory.
    */

    Destory();

    _width = width;
    _height = height;
    _format = format;

    GLint internalFormat;
    GLenum inFormat;
    GLenum type;
    bool isColorFormat = true;
    uint8 pixelSize = 0; 
    bool isFloatData = false; 

    switch (_format)
    {
    case TF_R8G8B8:
        inFormat = GL_RGB;
        internalFormat = GL_RGB;
        type = GL_UNSIGNED_BYTE;
        pixelSize = 3;
        isColorFormat = true;
        break;
    case TF_R5G6B5:
        inFormat = GL_RGB;
        internalFormat = GL_RGB;
        type = GL_UNSIGNED_SHORT_5_6_5;
        pixelSize = 2;
        isColorFormat = true;
        break;
    case TF_R8G8B8A8:
        {
            inFormat = GL_RGBA;
            internalFormat = GL_RGBA;
            type = GL_UNSIGNED_BYTE;
            pixelSize = 4;
            isColorFormat = true;
        }
        break;
    case TF_R5G5B5A1:
        inFormat = GL_RGBA;
        internalFormat = GL_RGBA;
        type = GL_UNSIGNED_SHORT_5_5_5_1;
        pixelSize = 2;
        isColorFormat = true;
        break;
    case TF_R4G4B4A4:
        inFormat = GL_RGBA;
        internalFormat = GL_RGBA;
        type = GL_UNSIGNED_SHORT_4_4_4_4;
        pixelSize = 2;
        isColorFormat = true;
        break;
    case TF_R32F:
        if (useToRenderTarget)
        {
#ifdef USE_OPENGLES
            inFormat = GL_RED_EXT;
            internalFormat = GL_R32F_EXT;
            type = GL_FLOAT;
#else
            inFormat = GL_DEPTH_COMPONENT;
            internalFormat = GL_DEPTH_COMPONENT;
            type = GL_UNSIGNED_INT;
#endif

        }else
        {
            inFormat = GL_LUMINANCE;
            internalFormat = GL_LUMINANCE;
            type = GL_FLOAT;
        }
        pixelSize = 4;
        isColorFormat = true;
        isFloatData = true;
        break;
    case TF_R16F:
        inFormat = GL_LUMINANCE;
        internalFormat = GL_LUMINANCE;
        type = GL_FLOAT;
        pixelSize = 1;
        isColorFormat = true;
        isFloatData = true;
        break;
    case TF_D24S8:
        inFormat = GL_LUMINANCE;
#ifndef GL_ES_VERSION_2_0
        internalFormat = GL_DEPTH24_STENCIL8;
#else
        internalFormat = GL_DEPTH24_STENCIL8_OES;
#endif // USE_OPENGLES
        type = GL_FLOAT;
        isColorFormat = false;
        isFloatData = true;
        break;
    case TF_D16:
        inFormat = GL_LUMINANCE;
        internalFormat = GL_DEPTH_COMPONENT16;
        type = GL_FLOAT;
        isColorFormat = false;
        isFloatData = true;
        break;
    default:
        break;
    }

    _impl = new atgTextureImpl;
    _impl->pixelSize = pixelSize;
    _impl->isFloatData = isFloatData;
    _impl->internalFormat = internalFormat;
    _impl->inFormat = inFormat;
    _impl->type = type;
    _impl->isColorFormat = isColorFormat;

    if (isColorFormat)
    {
        GL_ASSERT( glGenTextures(1, &_impl->TextureID) );
        GL_ASSERT( glBindTexture(GL_TEXTURE_2D, _impl->TextureID) );

        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST) );
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE) );
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE) );

        GL_ASSERT( glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, inFormat, type, pData) );

        if (!_impl->isFloatData && !useToRenderTarget)
        {
            GL_ASSERT( glGenerateMipmap(GL_TEXTURE_2D) ); // 生产mipmap,这个代码要放后面
        }

        GL_ASSERT( glBindTexture(GL_TEXTURE_2D, 0) );
    }
    else if(!isColorFormat && useToRenderTarget)
    {
        GL_ASSERT( glGenRenderbuffers(1, &_impl->TextureID) );  
        GL_ASSERT( glBindRenderbuffer(GL_RENDERBUFFER, _impl->TextureID) );
        GL_ASSERT( glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, _width, _height) );
        GL_ASSERT( glBindRenderbuffer(GL_RENDERBUFFER, 0) );
    }

    atgGpuResource::ReSet();
    if (!useToRenderTarget)
    {
        g_Renderer->InsertGpuResource(this, static_cast<GpuResDestoryFunc>(&atgTexture::Destory));
    }

    //LOG("create a atgTexture[%p].\n", this);
    SetFilterMode(TFM_FILTER_BILINEAR);
    return true;
}

bool atgTexture::Destory()
{
    atgGpuResource::Lost();
    SAFE_DELETE(_impl);
    
    return true;
}

atgTexture::LockData atgTexture::Lock()
{
    AASSERT(_impl);
    LockData lookData;
    lookData.pAddr = NULL;
    lookData.pitch = 0;
    if (!IsLost() && _impl)
    {
        if (_impl->pLockMemory == NULL)
        {
            if (_impl->isFloatData)
            {
                _impl->pLockMemory = (char*)(new float[_width * _height * _impl->pixelSize]);
            }
            else
            {
                _impl->pLockMemory = (char*)(new uint8[_width * _height * _impl->pixelSize]);
            }
        }

        lookData.pAddr = _impl->pLockMemory;
        _impl->locked = true;
    }

    return lookData;
}

void atgTexture::Unlock()
{
    if(_impl->locked)
    {
        GL_ASSERT( glBindTexture(GL_TEXTURE_2D, _impl->TextureID) );
        GL_ASSERT( glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, _impl->inFormat, _impl->type, _impl->pLockMemory) );
        GL_ASSERT( glBindBuffer(GL_ARRAY_BUFFER, 0));

        _impl->locked = false;
    }
}

bool atgTexture::IsLocked() const
{
    return _impl->locked;
}

bool atgTexture::Bind(uint8 index)
{
    if (_impl)
        return _impl->Bind(index);

    return false;
}

void atgTexture::Unbind(uint8 index)
{
    if (_impl)
    {
        _impl->Unbind(index);
    }
}

void atgTexture::SetFilterMode(TextureFilterMode filter)
{
    AASSERT(_impl);
    _filter = filter;
    GL_ASSERT( glBindTexture(GL_TEXTURE_2D, _impl->TextureID) );
    switch (filter)
    {
    case TFM_FILTER_DEFAULT:
    case TFM_FILTER_NEAREST:
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST) );
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
        break;
    case TFM_FILTER_BILINEAR:
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST) );
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        break;
    case TFM_FILTER_TRILINEAR:
    case TFM_FILTER_ANISOTROPIC:
#ifdef GL_TEXTURE_MAX_ANISOTROPY_EXT
        GL_ASSERT( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, (float)1.0f) ); // 1.0f, 2.0f, 4.0f ?
#endif // GL_TEXTURE_MAX_ANISOTROPY_EXT
        break;
    case TFM_FILTER_NOT_MIPMAP_ONLY_LINEAR:
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        break;
    case MAX_FILTERMODES:
        break;
    default:
        break;
    }
    GL_ASSERT( glBindTexture(GL_TEXTURE_2D, 0) );
}
void atgTexture::SetAddressMode(TextureCoordinate coordinate, TextureAddressMode address)
{
    AASSERT(_impl);
    GL_ASSERT( glBindTexture(GL_TEXTURE_2D, _impl->TextureID) );
    _address[coordinate] = address;
    GLenum uv = coordinate == TC_COORD_U ? GL_TEXTURE_WRAP_S : GL_TEXTURE_WRAP_T;
    switch (address)
    {
    case TAM_ADDRESS_WRAP:
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, uv, GL_REPEAT) );
        break;
    case TAM_ADDRESS_MIRROR:
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, uv, GL_MIRRORED_REPEAT) );
        break;
    case TAM_ADDRESS_CLAMP:
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, uv, GL_CLAMP_TO_EDGE) );
        break;
    case TAM_ADDRESS_BORDER:
#ifdef GL_ES_VERSION_2_0
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, uv, GL_CLAMP_TO_EDGE) );
#else
        GL_ASSERT( glTexParameteri(GL_TEXTURE_2D, uv, GL_CLAMP_TO_BORDER) );
#endif // GL_ES_VERSION_2_0
        break;
    case MAX_ADDRESSMODES:
        break;
    default:
        break;
    }
    GL_ASSERT( glBindTexture(GL_TEXTURE_2D, 0) );
}

atgResourceShader::atgResourceShader(ResourceShaderType Type):compiled(false)
{
    type = Type;
}

atgResourceShader::~atgResourceShader()
{
}

class atgVertexShaderImpl
{
public:
    atgVertexShaderImpl():vertexShaderID(0),vertexShaderCode(0){}
    ~atgVertexShaderImpl()
    {
        SAFE_DELETE_ARRAY(vertexShaderCode);
        if(vertexShaderID)
        {
            GL_ASSERT(glDeleteShader(vertexShaderID));
            vertexShaderID = 0;
        }
    }
public:
    GLuint vertexShaderID;
    GLchar* vertexShaderCode;
};

atgVertexShader::atgVertexShader():atgResourceShader(RST_Vertex),_impl(NULL)
{
}

atgVertexShader::~atgVertexShader()
{
    Destory();
}

bool atgVertexShader::LoadFromFile(const char* SourceFilePath)
{
    if(IsCompiled())
    {
        LOG("vs already loaded.");
        return false;
    }

    if (!SourceFilePath)
    {
        return false;
    }

    char* pVertexShaderCode = NULL;
    bool result;
    // Read the Vertex Shader code from the file
    result = ReadShadeFile(&pVertexShaderCode, SourceFilePath);
    if(!result)
    {
        LOG("Load vertex shader failure: %s\n", SourceFilePath);
        AASSERT(0);
        return false;
    }
    return LoadFromSource(pVertexShaderCode);
}

bool atgVertexShader::LoadFromSource( const char* Source )
{
    Destory();

    if(IsCompiled())
    {
        LOG("vs already loaded.");
        return false;
    }

    if(!_impl)
    {
        _impl = new atgVertexShaderImpl;
    }
    if(!Source)
    {
        return false;
    }
    _impl->vertexShaderCode = (GLchar*)Source;
    bool rs =  Compile();
    return rs;
}

bool atgVertexShader::Compile()
{
    if(!_impl || !_impl->vertexShaderCode)
    {
        return false;
    }
    GL_ASSERT( _impl->vertexShaderID = glCreateShader(GL_VERTEX_SHADER) );
    GLint compileResult;
    int infoLogLength;
    // Compile Vertex Shader
    //printf("Compiling shader : %s\n", SourceFilePath);
    const GLchar* vertexShaderCodeCst = _impl->vertexShaderCode;
    GL_ASSERT( glShaderSource(_impl->vertexShaderID, 1, &vertexShaderCodeCst, NULL) );
    GL_ASSERT( glCompileShader(_impl->vertexShaderID) );

    // Check Vertex Shader
    GL_ASSERT( glGetShaderiv(_impl->vertexShaderID, GL_COMPILE_STATUS, &compileResult) );
    if(compileResult != GL_TRUE)
    {
        LOG("Compile fail. compileResult=%d\n", compileResult);
        GL_ASSERT( glGetShaderiv(_impl->vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength) );
        if ( infoLogLength > 0 ){
            GLchar* errorInfo = new GLchar[infoLogLength + 1];
            glGetShaderInfoLog(_impl->vertexShaderID, infoLogLength, NULL, errorInfo);
            LOG("%s\n", errorInfo);
            delete [] errorInfo;
        }
        return false;
    }
    
    atgGpuResource::ReSet();
    //g_Renderer->InsertGpuResource(this, static_cast<GpuResDestoryFunc>(&atgVertexShader::Destory));
    LOG("VertexShader Compile success. compileResult=%d\n", compileResult);
    return true;
}

bool atgVertexShader::Destory()
{
    atgGpuResource::Lost();
    SAFE_DELETE(_impl);

    return true;
}

class atgFragmentShaderImpl
{
public:
    atgFragmentShaderImpl():fragmentShaderID(0),fragmentShaderCode(0){}
    ~atgFragmentShaderImpl()
    {
        SAFE_DELETE_ARRAY(fragmentShaderCode);
        if (fragmentShaderID)
        {
            GL_ASSERT( glDeleteShader(fragmentShaderID) );
            fragmentShaderID = 0;
        }
    }
public:
    GLuint fragmentShaderID;
    GLchar* fragmentShaderCode;
};

atgFragmentShader::atgFragmentShader():atgResourceShader(RST_Fragment),_impl(NULL)
{
}

atgFragmentShader::~atgFragmentShader()
{
    Destory();
}

bool atgFragmentShader::LoadFromFile(const char* SourceFilePath)
{
    if (!SourceFilePath)
    {
        return false;
    }

    char* pFragmentShaderCode = NULL;
    bool result;
    // Read the Vertex Shader code from the file
    result = ReadShadeFile(&pFragmentShaderCode, SourceFilePath);
    if(!result)
    {
        LOG("Load vertex shader failure: %s\n", SourceFilePath);
        AASSERT(0);
        return false;
    }
    return LoadFormSource(pFragmentShaderCode);
}

bool atgFragmentShader::LoadFormSource( const char* Source )
{
    Destory();

    if(!_impl)
    {
        _impl = new atgFragmentShaderImpl;
    }

    if(!Source)
    {
        return false;
    }
    _impl->fragmentShaderCode = (GLchar*)Source;
    return Compile();
}

bool atgFragmentShader::Compile()
{
    if(!_impl || !_impl->fragmentShaderCode)
    {
        return false;
    }
    GL_ASSERT( _impl->fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER) );
    GLint compileResult;
    int infoLogLength;
    //printf("Compiling shader : %s\n", SourceFilePath);
    const GLchar* fragmentShaderCodeCst = _impl->fragmentShaderCode;
    GL_ASSERT( glShaderSource(_impl->fragmentShaderID, 1, &fragmentShaderCodeCst, NULL) );
    GL_ASSERT( glCompileShader(_impl->fragmentShaderID) );

    // Check Vertex Shader
    GL_ASSERT( glGetShaderiv(_impl->fragmentShaderID, GL_COMPILE_STATUS, &compileResult) );
    if(compileResult != GL_TRUE)
    {
        GL_ASSERT( glGetShaderiv(_impl->fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength) );
        if ( infoLogLength > 0 ){
            GLchar* errorInfo = new GLchar[infoLogLength + 1];
            GL_ASSERT( glGetShaderInfoLog(_impl->fragmentShaderID, infoLogLength, NULL, errorInfo) );
            LOG("%s\n", errorInfo);
            delete [] errorInfo;
        }
        return false;
    }

    atgGpuResource::ReSet();
    //g_Renderer->InsertGpuResource(this, static_cast<GpuResDestoryFunc>(&atgFragmentShader::Destory));
    LOG("FragmentShader Compile success. compileResult=%d\n", compileResult);
    return true;
}

bool atgFragmentShader::Destory()
{
    atgGpuResource::Lost();
    SAFE_DELETE(_impl);

    return true;
}

//////////////////////////////////////////////////////////////////////////
class atgPassImpl
{
public:
    atgPassImpl():programID(0),pVS(0),pFS(0){}
    ~atgPassImpl() 
    { 
        SAFE_DELETE(pVS); 
        SAFE_DELETE(pFS); 
        if (programID)
        {
            GL_ASSERT( glDeleteProgram(programID) ); 
            programID = 0;
        }
    }

    bool Bind()
    {
        GL_ASSERT( glUseProgram(programID) );
        return true;
    }

    void Unbind()
    {
        GL_ASSERT( glUseProgram(0) );
    }

public:
    GLuint programID;
    atgVertexShader* pVS;
    atgFragmentShader* pFS;
};

atgPass::atgPass():_impl(0)
{
}

atgPass::~atgPass()
{
    Destory();
    g_Renderer->RemoveGpuResource(this);
}

const char* atgPass::GetName()
{
    return _name.c_str();
}

bool atgPass::Create( const char* VSFilePath, const char* FSFilePath )
{
    Destory();

    _impl = new atgPassImpl;
    _impl->pVS = new atgVertexShader();
    if(!_impl->pVS->LoadFromFile(VSFilePath))
    {
        LOG("load vertex shader fail.\n");
        return false;
    }

    _impl->pFS = new atgFragmentShader();
    if (!_impl->pFS->LoadFromFile(FSFilePath))
    {
        LOG("load pixel shader fail.\n");
        return false;
    }

    GL_ASSERT( _impl->programID = glCreateProgram() );

    if(!Link())
    {
        return false;
    }

    atgGpuResource::ReSet();
    g_Renderer->InsertGpuResource(this, static_cast<GpuResDestoryFunc>(&atgPass::Destory));
    //LOG("create a atgPass[%p](vs=%s,fs=%s).\n",this, VSFilePath, FSFilePath);

    //
    //GLint activeAttributes;
    //GLint length;
    //GL_ASSERT( glGetProgramiv(_impl->programID, GL_ACTIVE_ATTRIBUTES, &activeAttributes) );
    //if (activeAttributes > 0)
    //{
    //    GL_ASSERT( glGetProgramiv(_impl->programID, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &length) );
    //    if (length > 0)
    //    {
    //        GLchar* attribName = new GLchar[length + 1];
    //        GLint attribSize;
    //        GLenum attribType;
    //        GLint attribLocation;
    //        for (int i = 0; i < activeAttributes; ++i)
    //        {
    //            // Query attribute info.
    //            GL_ASSERT( glGetActiveAttrib(_impl->programID, i, length, NULL, &attribSize, &attribType, attribName) );
    //            attribName[length] = '\0';

    //            // Query the pre-assigned attribute location.
    //            GL_ASSERT( attribLocation = glGetAttribLocation(_impl->programID, attribName) );

    //            // Assign the vertex attribute mapping for the effect.
    //            //effect->_vertexAttributes[attribName] = attribLocation;
    //        }
    //        SAFE_DELETE_ARRAY(attribName);
    //    }
    //}

    //{
    //    GLint activeUniforms;
    //    GLint length;
    //    GL_ASSERT( glGetProgramiv(_impl->programID, GL_ACTIVE_UNIFORMS, &activeUniforms) );
    //    if (activeUniforms > 0)
    //    {
    //        GL_ASSERT( glGetProgramiv(_impl->programID, GL_ACTIVE_UNIFORM_MAX_LENGTH, &length) );
    //        if (length > 0)
    //        {
    //            GLchar* uniformName = new GLchar[length + 1];
    //            GLint uniformSize;
    //            GLenum uniformType;
    //            GLint uniformLocation;
    //            for (int i = 0; i < activeUniforms; ++i)
    //            {
    //                // Query attribute info.
    //                GL_ASSERT( glGetActiveUniform(_impl->programID, i, length, NULL, &uniformSize, &uniformType, uniformName) );
    //                uniformName[length] = '\0';

    //                // Query the pre-assigned attribute location.
    //                GL_ASSERT( uniformLocation = glGetAttribLocation(_impl->programID, uniformName) );

    //                // Assign the vertex attribute mapping for the effect.
    //                //effect->_vertexAttributes[attribName] = attribLocation;
    //            }
    //            SAFE_DELETE_ARRAY(uniformName);
    //        }
    //    }
    //}


    return true;
}

bool atgPass::Destory()
{
    atgGpuResource::Lost();
    SAFE_DELETE(_impl);

    return true;
}

bool atgPass::Bind()
{
    if (_impl)
    {
        bool rs = _impl->Bind();
        if (rs)
        {
            BeginContext(NULL);
        }
        return rs;
    }

    return false;
}

void atgPass::Unbind()
{
    if (_impl)
    {
        EndContext(NULL);
        _impl->Unbind();
    }
}

bool atgPass::Link()
{
    AASSERT(_impl);

    GL_ASSERT( glAttachShader(_impl->programID, _impl->pVS->_impl->vertexShaderID) );
    GL_ASSERT( glAttachShader(_impl->programID, _impl->pFS->_impl->fragmentShaderID) );

    int infoLogLength;
    GL_ASSERT( glLinkProgram(_impl->programID) );
    // Check the program
    GLint linkResult;
    GL_ASSERT( glGetProgramiv(_impl->programID, GL_LINK_STATUS, &linkResult) );
    if(linkResult != GL_TRUE)
    {
        LOG("_impl->programID=%d, linkResult=%d\n",_impl->programID, linkResult);
        GL_ASSERT( glGetProgramiv(_impl->programID, GL_INFO_LOG_LENGTH, &infoLogLength) );
        if ( infoLogLength > 0 ){
            GLchar* errorInfo = new GLchar[infoLogLength + 1];
            GL_ASSERT( glGetProgramInfoLog(_impl->programID, infoLogLength, NULL, errorInfo) );
            LOG("%s\n", errorInfo);
            delete [] errorInfo;
        }
        return false;
    }
    return true;
}

void atgPass::BeginContext(void* data)
{
    GLuint programID = _impl->programID;
    if(programID != 0)
    {
        atgMatrix WVP;
        atgMatrix Wrld;
        atgMatrix View;
        atgMatrix Proj;
        g_Renderer->GetMatrix(Wrld, MD_WORLD);
        g_Renderer->GetMatrix(View, MD_VIEW);
        g_Renderer->GetMatrix(Proj, MD_PROJECTION);
        WVP = Proj * View * Wrld;
        SetMatrix4x4(UNF_M_WVP, WVP);
    }
}

void atgPass::EndContext(void* data)
{

}

bool atgPass::SetInt(const char* var_name, int value)
{
    GLint identityID = 0;
    GL_ASSERT( identityID = glGetUniformLocation(_impl->programID, var_name) );
    if(identityID != -1)
    {
        GL_ASSERT( glUniform1i(identityID, value) );
        return true;
    }
    return false;
}

bool atgPass::SetFloat(const char* var_name, float value)
{
    GLint identityID = 0;
    GL_ASSERT( identityID = glGetUniformLocation(_impl->programID, var_name) );
    if(identityID != -1)
    {
        GL_ASSERT( glUniform1f(identityID, value) );
        return true;
    }
    return false;
}

bool atgPass:: SetFloat2(const char* var_name, const atgVec2& f)
{
    GLint identityID = 0;
    GL_ASSERT( identityID = glGetUniformLocation(_impl->programID, var_name) );
    if(identityID != -1)
    {
        GL_ASSERT( glUniform2f(identityID, f.x, f.y) );
        return true;
    }
    return false;
}


bool atgPass::SetFloat3(const char* var_name, const atgVec3& f)
{
    GLint identityID = 0;
    GL_ASSERT( identityID = glGetUniformLocation(_impl->programID, var_name) );
    if(identityID != -1)
    {
        GL_ASSERT( glUniform3f(identityID, f.x, f.y, f.z) );
        return true;
    }
    return false;
}

bool atgPass::SetFloat4(const char* var_name, const atgVec4& f)
{
    GLint identityID = 0;
    GL_ASSERT( identityID = glGetUniformLocation(_impl->programID, var_name) );
    if(identityID != -1)
    {
        GL_ASSERT( glUniform4f(identityID, f.x, f.y, f.z, f.w) );
        return true;
    }
    return false;
}

bool atgPass::SetMatrix4x4(const char* var_name, const atgMatrix& mat)
{
    // OPENGLES 不允许第三个参数为GL_TRUE， 所以自己转置矩阵
    //glUniformMatrix4fv(MVPMatrixID, 1, GL_FALSE, (GLfloat*)MVP.m); 
    atgMatrix matTemp(mat);
    matTemp.Transpose();

    GLint identityID = 0;
    GL_ASSERT( identityID = glGetUniformLocation(_impl->programID, var_name) );
    if(identityID != -1)
    {
        glUniformMatrix4fv(identityID, 1, GL_FALSE, (GLfloat*)&matTemp.m[0]);
        return true;
    }
    return false;
}

bool atgPass::SetTexture(const char* var_name, uint8 index)
{
    GLuint identityID = 0;
    GL_ASSERT( identityID = glGetUniformLocation(_impl->programID, var_name) );
    if (identityID != -1)
    {
        glUniform1i(identityID, index);
        return true;
    }
    return false;
}

uint8 atgPass::GetTexture(const char* var_name)
{
    return (uint8)-1;
}

class atgRenderTargetImpl
{
public:
    atgRenderTargetImpl():FrameBufferId(0),PreviousFBO(0),width(0),height(0) { }
    ~atgRenderTargetImpl() {  glBindFramebuffer(GL_FRAMEBUFFER, 0); glDeleteFramebuffers(1, &FrameBufferId); }

    bool Bind()
    {
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &PreviousFBO);
        GL_ASSERT( glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferId) );

        g_Renderer->GetViewPort(viewPort[0], viewPort[1], viewPort[2], viewPort[3]);
        g_Renderer->SetViewPort(0, 0, width, height);
        return true;
    }

    void Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, PreviousFBO);
        g_Renderer->SetViewPort(viewPort[0], viewPort[1], viewPort[2], viewPort[3]);
    }

    GLuint FrameBufferId;
    GLint PreviousFBO;
    uint32 width;
    uint32 height;
    uint32 viewPort[4];
};

atgRenderTarget::atgRenderTarget():_impl(NULL)
{
}

atgRenderTarget::~atgRenderTarget()
{
    Destory();
    g_Renderer->RemoveGpuResource(this);
}

bool atgRenderTarget::Create( std::vector<atgTexture*>& colorBuffer, atgTexture* depthStencilBuffer )
{
    if (colorBuffer.empty())
    {
        LOG("atgRenderTarget create failture. color buffer is empty.\n");
        return false;
    }

    if (NULL == depthStencilBuffer)
    {
        LOG("atgRenderTarget create failture. depthStencil buffer is NULL.\n");
        return false;
    }
    
    if (colorBuffer[0]->GetWidth() != depthStencilBuffer->GetWidth() ||
        colorBuffer[0]->GetHeight() != depthStencilBuffer->GetHeight() )
    {
        LOG("atgRenderTarget create failture. colorBuffer buffer and depthStencil Buffer size must same.\n");
        return false;
    }

    Destory();

    _impl = new atgRenderTargetImpl();
    _colorBuffer.resize(colorBuffer.size());
    std::copy(colorBuffer.begin(), colorBuffer.end(), _colorBuffer.begin());

    _depthStencilBuffer = depthStencilBuffer;
    _impl->width = depthStencilBuffer->GetWidth();
    _impl->height = depthStencilBuffer->GetHeight();

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &_impl->PreviousFBO);
    GL_ASSERT( glGenFramebuffers(1, &_impl->FrameBufferId) );
    GL_ASSERT( glBindFramebuffer(GL_FRAMEBUFFER, _impl->FrameBufferId) );

    if (colorBuffer[0]->_impl->internalFormat == GL_DEPTH_COMPONENT)
    {
        GL_ASSERT( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, colorBuffer[0]->_impl->TextureID, 0) );
    }
    else
    {
        GL_ASSERT( glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer[0]->_impl->TextureID, 0) );
        GL_ASSERT( glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthStencilBuffer->_impl->TextureID) );
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        LOG("Error: FrameBufferObject is not complete!\n");
        GL_ASSERT(0);
    }

    GL_ASSERT( glBindFramebuffer(GL_FRAMEBUFFER, _impl->PreviousFBO) );

    atgGpuResource::ReSet();
    //g_Renderer->InsertGpuResource(this, static_cast<GpuResDestoryFunc>(&atgRenderTarget::Destory));
    //LOG("create a atgRenderTarget[%p].\n",this);

    return true;
}

bool atgRenderTarget::Destory()
{
    atgGpuResource::Lost();
    SAFE_DELETE(_impl);

    return true;
}

bool atgRenderTarget::Bind(uint8 index)
{
    if (IsLost() && !_colorBuffer.empty())
    {
        if (_depthStencilBuffer->IsLost())
        {
            if(false == _depthStencilBuffer->Create(_depthStencilBuffer->_width, _depthStencilBuffer->_height, _depthStencilBuffer->_format, NULL, true))
            {
                return false;
            }
        }

        if (_colorBuffer[0]->IsLost())
        {
            if(false == _colorBuffer[0]->Create(_colorBuffer[0]->_width, _colorBuffer[0]->_height, _colorBuffer[0]->_format, NULL, true))
            {
                return false;
            }
        }

        if(false == Create(_colorBuffer, _depthStencilBuffer))
        {
            return false;
        }
    }

    if (_impl)
    {
        return _impl->Bind();
    }

    return false;
}

void atgRenderTarget::Unbind()
{
    if (_impl)
    {
        _impl->Unbind();
    }
}

bool atgRenderer::Initialize( uint32 width, uint32 height, uint8 bpp )
{
    bool rt;
#ifdef _WIN32
        rt = win32_init_ogl();
#elif defined _ANDROID
        rt = android_init_ogl();
#endif


#ifndef GL_ES_VERSION_2_0
    std::string vendorName("OpenGL ");
#else
    std::string vendorName;
#endif // !GL_ES_VERSION_2_0

    vendorName += (const char*)glGetString(GL_VERSION);
    vendorName += "\n\t\t";
    vendorName += (const char*)glGetString(GL_VENDOR);
    vendorName += "\n\t\t";
    vendorName += (const char*)glGetString(GL_RENDERER);
    vendorName += "\n\t\t";
    vendorName += (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION); 
    vendorName += "\n\t\t";
    vendorName += (const char*)glGetString(GL_EXTENSIONS);

    //> 取得支持压缩纹理的格式,大多数用不上
    //GLint formatCount;
    //glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &formatCount);
    //GLint* formatArray = new GLint[formatCount];
    //glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, formatArray);
    //delete formatArray;

    LOG("%s\n", vendorName.c_str());

    if (!rt)
    {
        LOG("init opengl failure!\n");
        return false;
    }
#ifdef OPENGL_FIX_PIPELINE
    glMatrixMode(GL_PROJECTION);                        // Select The Projection Matrix
    glLoadIdentity();                                   // Reset The Projection Matrix
    {
        // Method1. Calculate The Aspect Ratio Of The Window
        //gluPerspective(90.0f,(GLfloat)width/(GLfloat)height,0.1f,1000.0f);

        // Method2. OpenGL self function;
        //float aspect = width * 1.0f / height;
        //glFrustum(-1.0f, 1.0f, -(1.0f / aspect), (1.0f / aspect), 0.1f, 1000.0f);

        // Method3. Make slef matrix;
        float aspect = width * 1.0f / height;
        Matrix proj;
        Perspective(proj.m, 90.0f, aspect, 0.001f, 1000.0f);
        glLoadMatrixf((GLfloat*)proj.m);

        // Get Matrix Mode can call glGet* function; example
        //GLfloat ResultMatrix[16];
        //glGetFloatv(GL_PROJECTION_MATRIX, &ResultMatrix[0]);
        //for(int i=0; i < 16; i+=4)
        //{
        //    printf("%f, %f, %f, %f\n", ResultMatrix[i], ResultMatrix[i + 1],
        //        ResultMatrix[i + 2], ResultMatrix[i + 3]);
        //}
    }

    glMatrixMode(GL_MODELVIEW);                         // Select The Modelview Matrix
    glLoadIdentity();                                   // Reset The Modelview Matrix
    glShadeModel(GL_SMOOTH);                            // Enable Smooth Shading
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  // Really Nice Perspective Calculations
#endif // OPENGL_FIX_PIPELINE
    GL_ASSERT( glEnable(GL_DEPTH_TEST) );                            // Enables Depth Testing

    //>The Type Of Depth Testing To Do
    //>默认是小于(GL_LESS). 使用小于等于.也是和directx保持一致.
    GL_ASSERT( glDepthFunc(GL_LEQUAL) );                             
                  
    //>opengl 默认使用逆时针(counter-clockwise)的顶点组成的面作为前面.
    //>directx 默认剔除逆时针的顶点组成的面.即认为ccw作为后面.
    //>所以这里opengl设置剔除前面.就保证了调用SetFaceCull的统一.
    GL_ASSERT( glCullFace(GL_FRONT) );

    LOG("|||| Graphic Driver ===> Using %s!\n", atgRenderer::getName());
    return true;
}

void atgRenderer::Shutdown()
{
    ReleaseAllGpuResource();
#ifdef _WIN32
    win32_shutdown_ogl();
#endif
}

bool atgRenderer::Resize( uint32 width, uint32 height )
{
    return true;
}

void atgRenderer::SetViewPort( uint32 offsetX, uint32 offsetY, uint32 width, uint32 height )
{
    glViewport(offsetX,offsetY,width,height);                       // Reset The Current Viewport

    _VP_offsetX = offsetX;
    _VP_offsetY = offsetY;
    _VP_width = width;
    _VP_height = height;
}

void atgRenderer::GetViewPort(uint32& offsetX, uint32& offsetY, uint32& width, uint32& height) const
{
    offsetX = _VP_offsetX;
    offsetY = _VP_offsetY;
#ifdef _WIN32
    RECT rect;
    GetClientRect(g_hWnd, &rect);
    width  = rect.right - rect.left;
    height = rect.bottom - rect.left;
#else
    width  = _VP_width;
    height = _VP_height;
#endif // _WIN32

}

void atgRenderer::SetMatrix(MatrixDefine index, const atgMatrix& mat)
{
    switch(index)
    {
    case MD_WORLD:
        {
            _matrixs[MD_WORLD] = mat;
            break;
        }
    case MD_VIEW:
        {
            _matrixs[MD_VIEW] = mat;
#ifdef OPENGL_FIX_PIPELINE
            Matrix temp;
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            _matrixs[MD_VIEW].Concatenate(_matrixs[MD_WORLD], temp);
            glLoadMatrixf((GLfloat*)&temp);
#endif
            break;
        }
    case MD_PROJECTION:
        {
            _matrixs[MD_PROJECTION] = mat;
#ifdef OPENGL_FIX_PIPELINE
            glMatrixMode(GL_PROJECTION);                        // Select The Projection Matrix
            glLoadIdentity();                                   // Reset The Projection Matrix
            glLoadMatrixf((GLfloat*)&_matrixs[MD_PROJECTION]);
#endif
            break;
        }
    default: break;
    }
}

void atgRenderer::GetMatrix(atgMatrix& mat, MatrixDefine index) const
{
    if (MD_WORLD <= index && index < MD_NUMBER)
    {
        mat = _matrixs[index];
    }
}

void atgRenderer::SetLightEnable(bool enable)
{
//    if (enable)
//    {
//        glEnable(GL_LIGHTING);
//    }else
//    {
//        glDisable(GL_LIGHTING);
//    }
}

void atgRenderer::SetDepthTestEnable(bool enable)
{
    if (enable)
    {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
    }
    else
    {
        glDisable(GL_DEPTH_TEST);
    }
}

void atgRenderer::SetAlphaTestEnable(bool enbale, float value)
{
#ifdef OPENGL_FIX_PIPELINE
    if (enbale)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, value);
    }
    else
    {
        glDisable(GL_ALPHA_TEST);
    }
#endif // OPENGL_FIX_PIPELINE
}

void atgRenderer::SetFaceCull(FaceCullMode mode)
{
    if (mode == FCM_NONE)
    {
        GL_ASSERT( glDisable(GL_CULL_FACE) );
    }
    else if(mode == FCM_CW)
    {
        GL_ASSERT( glEnable(GL_CULL_FACE) );
        GL_ASSERT( glFrontFace(GL_CW) );
    }else
    {
        GL_ASSERT( glEnable(GL_CULL_FACE) );
        GL_ASSERT( glFrontFace(GL_CCW) );
    }
}

void atgRenderer::SetScissorEnable(bool enable, int x /* = 0 */, int y /* = 0 */, int w /* = 0 */, int h /* = 0 */)
{
    if (enable)
    {
        GL_ASSERT( glEnable(GL_SCISSOR_TEST) );
        GL_ASSERT( glScissor(x, y, w, h) );
    }
    else
    {
        GL_ASSERT( glDisable(GL_SCISSOR_TEST) );
    }
}

GLenum BlendFuncToGLBlendFunc(BlendFunction BlendFunc)
{
    switch(BlendFunc)
    {
    case BF_ZERO : return GL_ZERO;
    case BF_ONE : return GL_ONE;
    case BF_SRC_COLOR : return GL_SRC_COLOR;
    case BF_ONE_MINUS_SRC_COLOR : return GL_ONE_MINUS_SRC_COLOR;
    case BF_DST_COLOR : return GL_DST_COLOR;
    case BF_ONE_MINUS_DST_COLOR : return GL_ONE_MINUS_DST_COLOR;
    case BF_SRC_ALPHA : return GL_SRC_ALPHA;
    case BF_ONE_MINUS_SRC_ALPHA : return GL_ONE_MINUS_SRC_ALPHA;
    case BF_DST_ALPHA : return GL_DST_ALPHA;
    case BF_ONE_MINUS_DST_ALPHA : return GL_ONE_MINUS_DST_ALPHA;
    default:
        AASSERT(0);
        return 0;
    }
}

BlendFunction GLBlendFuncToEngineBlendFunc(GLenum BlendFunc)
{
    switch(BlendFunc)
    {
    case GL_ZERO : return BF_ZERO;
    case GL_ONE : return BF_ONE;
    case GL_SRC_COLOR : return BF_SRC_COLOR;
    case GL_ONE_MINUS_SRC_COLOR : return BF_ONE_MINUS_SRC_COLOR;
    case GL_DST_COLOR : return BF_DST_COLOR;
    case GL_ONE_MINUS_DST_COLOR : return BF_ONE_MINUS_DST_COLOR;
    case GL_SRC_ALPHA : return BF_SRC_ALPHA;
    case GL_ONE_MINUS_SRC_ALPHA : return BF_ONE_MINUS_SRC_ALPHA;
    case GL_DST_ALPHA : return BF_DST_ALPHA;
    case GL_ONE_MINUS_DST_ALPHA : return BF_ONE_MINUS_DST_ALPHA;
    default:
        AASSERT(0);
        return (BlendFunction)0;
    }
}

void atgRenderer::GetBlendFunction(BlendFunction& outSrcBlend, BlendFunction& outDestBlend)
{
    GLint GLSrc = 0;
    GLint GLDest = 0;
    GL_ASSERT( glGetIntegerv(GL_BLEND_SRC_RGB, &GLSrc) );
    GL_ASSERT( glGetIntegerv(GL_BLEND_DST_RGB, &GLDest) );
    outSrcBlend = GLBlendFuncToEngineBlendFunc(GLenum(GLSrc));
    outDestBlend = GLBlendFuncToEngineBlendFunc(GLenum(GLDest));
}

void atgRenderer::SetBlendFunction(BlendFunction SrcBlend, BlendFunction DestBlend)
{
    if (SrcBlend == BF_NONE || DestBlend == BF_NONE)
    {
        GL_ASSERT( glDisable(GL_BLEND) );

    }else
    {
        GLenum glSrc = BlendFuncToGLBlendFunc(SrcBlend);
        GLenum glDest = BlendFuncToGLBlendFunc(DestBlend);
        GL_ASSERT( glEnable(GL_BLEND) );
        GL_ASSERT( glBlendFunc(glSrc, glDest) );
    }
}

void atgRenderer::Clear(ClearTarget target)
{
    ATG_PROFILE("atgRenderer::Clear");
    GL_ASSERT( glClearColor(0.0f, 0.141f, 0.141f, 1.0f) );              // Set Black Background
    GL_ASSERT( glClearDepth(1.0f) );                                // Set Depth Buffer Setup
    GLbitfield bitMask = 0;
    if (target & CT_COLOR)
    {
        bitMask |= GL_COLOR_BUFFER_BIT;
    }
    if (target & CT_DEPTH)
    {
        bitMask |= GL_DEPTH_BUFFER_BIT;
    }
    if (target & CT_STENCIL)
    {
        bitMask |= GL_STENCIL_BUFFER_BIT;
    }
    GL_ASSERT( glClear( bitMask ) );// Clear Screen And Depth Buffer
}

void atgRenderer::BeginFrame()
{
}

void atgRenderer::EndFrame()
{

}

void atgRenderer::Present()
{
    ATG_PROFILE("atgRenderer::Present");
#ifdef _WIN32
    win32_ogl_present();
#elif defined _ANDROID
    android_ogl_present();
#endif
    
}

bool PrimitiveTypeConvertToOGL(PrimitveType pt, GLenum& gl_pt)
{
    switch(pt)
    {
    case PT_POINTS:
        gl_pt = GL_POINTS;
        break;
    case PT_LINES:
        gl_pt = GL_LINES;
        break;
    case PT_LINE_STRIP:
        gl_pt = GL_LINE_STRIP;
        break;
    case PT_TRIANGLES:
        gl_pt = GL_TRIANGLES;
        break;
    case PT_TRIANGLE_STRIP:
        gl_pt = GL_TRIANGLE_STRIP;
        break;
    case PT_TRIANGLE_FAN:
        gl_pt = GL_TRIANGLE_FAN;
        break;
    default:
        return false;
    }
    return true;
}

bool atgRenderer::DrawPrimitive( PrimitveType pt, uint32 primitveCount, uint32 verticesCount, uint32 offset )
{
    GLenum gl_pt;
    if(PrimitiveTypeConvertToOGL(pt, gl_pt)){
        GL_ASSERT( glDrawArrays(gl_pt, offset, verticesCount) );
    }
    return true;
}

bool atgRenderer::DrawIndexedPrimitive( PrimitveType pt, uint32 primitveCount,  uint32 indicesCount, uint32 verticesCount)
{
    //mode
    //    Specifies what kind of primitives to render. Symbolic constants GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, and GL_TRIANGLES are accepted.

    //count
    //    Specifies the number of elements to be rendered.

    //type
    //    Specifies the type of the values in indices. Must be GL_UNSIGNED_BYTE or GL_UNSIGNED_SHORT.

    //indices
    //    Specifies a pointer to the location where the indices are stored.

    GLenum gl_pt;
    if(PrimitiveTypeConvertToOGL(pt, gl_pt)){
        GL_ASSERT( glDrawElements(gl_pt, indicesCount, GL_UNSIGNED_SHORT, 0) ); // with VBO
    }
    return true;
}

void atgRenderer::SetPointSize(float size)
{
#if defined(_WIN32) && defined(__GLEW_H__)
    GLfloat pointSize = size;
    glPointSize(size);
#endif // !_ANDROID
}

#endif