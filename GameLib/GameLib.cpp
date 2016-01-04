// GameLib.cpp : �������̨Ӧ�ó������ڵ㡣
//
#ifdef _WIN32
#include "stdafx.h"
#endif // _WIN32

#include "atgGame.h"
#include "atgRenderer.h"
#include "atgCamera.h"
#include "atgIntersection.h"
#include "atgMisc.h"
//#include "perfMonitor.h"

static const int g_pointNumber = 1000;
static float g_points[3*g_pointNumber];

class Game : public atgGame
{
public :
	~Game()
	{
	}

    bool OnInitialize()
    {
        LOG("Game::OnInitialize!\n");

        //SetRandomSeed((uint32)GetAbsoluteMsecTime());
        //LoadConfig();

        g_Renderer->SetViewport(0, 0, GetWindowWidth(), GetWindowHeight());
        g_Renderer->SetFaceCull(FCM_CW);

        Matrix world(MatrixIdentity);
        //world.RotationZXY(0.0f, -90.0f, -90.0f);
        g_Renderer->SetMatrix(MD_WORLD, world);

        _Camera = new atgCamera();

        _Camera2 = new atgCamera();

        _Camera2->SetClipNear(10);
        _Camera2->SetClipFar(400);
        _Camera2->SetPosition(Vec3(0,150,300));

        _down = false;

        for (int i = 0; i < g_pointNumber; i+=3)
        {
            g_points[3*i]       = Random(-500.0f, 500.f);
            g_points[3*i + 1]   = Random(-500.0f, 500.f);
            g_points[3*i + 2]   = Random(-500.0f, 500.f);
        }

        return true;
    }

    virtual void OnFrame()
    {
        static int frame = 0;
        static float lastTime = GetAbsoluteMsecTime() / 1000.f;
        static float addTime = 0;
        float time = GetAbsoluteMsecTime() / 1000.f;
        float elapsedTime = static_cast<float>(time - lastTime);
        lastTime = time;
        addTime += elapsedTime;
        
        //float fps;
        //_monitor.Update(fps);

        if (addTime > 1.f)
        {
            addTime -= 1.f;
            LOG("FPS = %d, %lf\n", frame, elapsedTime);
            frame = 0;
            //LOG("_monitor FPS = %f\n", fps);
        }
        
        ++frame;


        if (_Camera)
        {
            g_Renderer->SetMatrix(MD_VIEW, _Camera->GetView());
            g_Renderer->SetMatrix(MD_PROJECTION, _Camera->GetProj());
        }

        g_Renderer->Clear();
        g_Renderer->BeginFrame();
        DrawAxis();
        g_Renderer->EndFrame();
        g_Renderer->Present();
    }


    void OnKeyScanDown(Key::Scan keyscan)
    {
        float moveSpeed = 10.5f;

        switch (keyscan)
        {
        case Key::W:
            {
                Vec3 forward = _Camera->GetForward();
                Vec3 pos = _Camera->GetPosition();
                forward.Scale(moveSpeed);
                pos.Add(forward.m);
                _Camera->SetPosition(pos.m);

            }
            break;
        case Key::S:
            {
                Vec3 forward = _Camera->GetForward();
                Vec3 pos = _Camera->GetPosition();
                forward.Scale(-moveSpeed);
                pos.Add(forward.m);
                _Camera->SetPosition(pos.m);
            }
            break;
        case Key::A:
            {
                Vec3 right = _Camera->GetRight();
                Vec3 pos = _Camera->GetPosition();
                right.Scale(moveSpeed);
                pos.Add(right.m);
                _Camera->SetPosition(pos.m);
            }
            break;
        case Key::D:
            {
                Vec3 right = _Camera->GetRight();
                Vec3 pos = _Camera->GetPosition();
                right.Scale(-moveSpeed);
                pos.Add(right.m);
                _Camera->SetPosition(pos.m);
            }
            break;
        default:
            break;
        }

        atgGame::OnKeyScanDown(keyscan);
    }


    void OnPointerDown(uint8 id, int16 x, int16 y)
    {
        if(id == 1)
        {
            // right button down
            _down = true;
        }

    }

    void OnPointerMove(uint8 id, int16 x, int16 y)
    {

        static int16 last_x = x;
        static int16 last_y = y;

        static float oYaw = atgMath::DegreesToRadians(180.f);
        static float oPitch = 0;//DegreesToRadians(45.f);

        if (_down)
        {
            if (_Camera)
            {
                float dx = static_cast<float>(x - last_x);
                float dy = static_cast<float>(y - last_y);
                oYaw -= dx * 0.001f;
                oPitch += dy * 0.001f;
                _Camera->SetYaw(oYaw);
                _Camera->SetPitch(oPitch);
            }
        }

        if (id == MBID_MIDDLE)
        {
            float moveSpeed = 10.5f;
            if (x > 0)
            {
                Vec3 forward = _Camera->GetForward();
                Vec3 pos = _Camera->GetPosition();
                atgMath::VecScale(forward.m, moveSpeed, forward.m);
                atgMath::VecAdd(pos.m, forward.m, pos.m);
                _Camera->SetPosition(pos.m);
            }
            else
            {
                Vec3 forward = _Camera->GetForward();
                Vec3 pos = _Camera->GetPosition();
                atgMath::VecScale(forward.m, -moveSpeed, forward.m);
                atgMath::VecAdd(pos.m, forward.m, pos.m);
                _Camera->SetPosition(pos.m);
            }
        }

        last_x = x;
        last_y = y;
    }

    void OnPointerUp(uint8 id, int16 x, int16 y)
    {
        if(id == 1)
        {
            _down = false;
        }
    }

    void DrawAxis()
    {

        g_Renderer->BeginQuad();
        g_Renderer->AddQuad(Vec3(0.f,0.0f,0.0f).m, Vec3(0.0f, 100.0f, 0.0f).m, Vec3(100.0f, 0.0f, 0.0f).m, Vec3(100.0f, 100.0f, 0.0f).m, Vec3Up.m);
        g_Renderer->AddQuad(Vec3(-10.f,-10.0f,0.0f).m, Vec3(0.0f, 60.0f, 0.0f).m, Vec3(60.0f, 0.0f, 0.0f).m, Vec3(150.0f, 150.0f, 0.0f).m, Vec3Right.m);
        g_Renderer->EndQuad();

        atgFrustum f;
        f.BuildFrustumPlanes(_Camera2->GetView().m,_Camera2->GetProj().m);
        f.DebugRender();
        
        /*
        g_Renderer->SetPointSize(5.0f);
        g_Renderer->BeginPoint();
        float point[3];
        for (int i = 0; i < g_pointNumber; i+=3)
        {
            point[0] = g_points[3*i];
            point[1] = g_points[3*i+1];
            point[2] = g_points[3*i+2];

            if (f.IsPointInFrustum(point))
            {
                //g_Renderer->SetPointSize(10.0f);
                g_Renderer->AddPoint(point, Vec3Up.m);
            }
            else
            {
                //g_Renderer->SetPointSize(5.0f);
                g_Renderer->AddPoint(point, Vec3Right.m);
            }
        }
        g_Renderer->EndPoint();
        */

        uint32 oldVP[4];
        g_Renderer->GetViewPort(oldVP[0], oldVP[1], oldVP[2], oldVP[3]);

        uint32 newVP[4] = {0, IsOpenGLGraph() ? 0 : oldVP[3] - 100, 100, 100 };

        g_Renderer->SetViewport(newVP[0], newVP[1], newVP[2], newVP[3]);

        Matrix oldWorld;
        g_Renderer->GetMatrix(oldWorld, MD_WORLD);
        Matrix newWorld(MatrixIdentity);
        g_Renderer->SetMatrix(MD_WORLD, newWorld);

        Matrix oldView;
        g_Renderer->GetMatrix(oldView, MD_VIEW);
        Matrix newView(MatrixIdentity);
        Vec3 p(0.0f, 0.0f, 3.5f);
        if (_Camera)
        {
            newView.SetColumn3(0, _Camera->GetRight());
            newView.SetColumn3(1, _Camera->GetUp());
            newView.SetColumn3(2, _Camera->GetForward());
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
        g_Renderer->SetViewport(oldVP[0], oldVP[1], oldVP[2], oldVP[3]);
    }

    class atgCamera* _Camera;
    class atgCamera* _Camera2;
    bool _down;

    //atgPerfMonitor _monitor;
};

#ifdef _WIN32

int _tmain(int argc, _TCHAR* argv[])
{
    Game game;
    if(game.Initialize(NULL))
    {
        game.Run();
        game.Shutdown();
    }


	return 0;
}

#endif