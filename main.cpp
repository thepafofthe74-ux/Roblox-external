// ESP + Aimbot + Fly | version-e7d81637d42c4b23
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <set>

namespace Off {
    constexpr uintptr_t FakeDM_Ptr=0x8d22868,FakeDM_RealDM=0x1f8;
    constexpr uintptr_t VE_Ptr=0x8351408,VE_VM=0x1b0;
    constexpr uintptr_t DM_WS=0x158,WS_CAM=0x4b8,CAM_POS=0xfc;
    constexpr uintptr_t INST_DESC=0x18,INST_CNAME=0x08;
    constexpr uintptr_t INST_NCONT=0x70,INST_NAME=0x08;
    constexpr uintptr_t INST_CVEC=0x78,VEC_BEG=0x00,VEC_END=0x08;
    constexpr uintptr_t STR_LEN=0x10;
    constexpr uintptr_t PL_LOCAL=0x130,PL_MODEL=0x298,PL_TEAM=0x2d8;
    constexpr uintptr_t HUM_HP=0x190,HUM_MAXHP=0x1a8,HUM_HRP=0x478;
    constexpr uintptr_t BP_PRIM=0x188,PRIM_POS=0xec,PRIM_VEL=0xf8;
}

struct Vec3{float x,y,z;};
struct Vec2{float x,y;};
struct VM{float m[4][4];};

// Settings
struct {
    std::atomic<bool> menu{false};
    std::atomic<bool> boxes{true},tracers{true},hp{true},names{true};
    std::atomic<bool> fly{false};
    std::atomic<bool> aimbot{false};
    std::set<std::string> selectedPlayers; // empty = all enemies
} G;

class Mem {
public:
    HANDLE proc=nullptr; uintptr_t base=0;
    bool Attach(const wchar_t* exe){
        PROCESSENTRY32W pe{sizeof(pe)};
        HANDLE s=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
        bool ok=false;
        while(Process32NextW(s,&pe)) if(!_wcsicmp(pe.szExeFile,exe)){
            proc=OpenProcess(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|PROCESS_QUERY_INFORMATION,0,pe.th32ProcessID);
            ok=proc!=nullptr; break;
        }
        CloseHandle(s); if(!ok)return false;
        HMODULE m; DWORD n;
        EnumProcessModules(proc,&m,sizeof(m),&n); base=(uintptr_t)m; return true;
    }
    template<typename T> T R(uintptr_t a)const{T v{};ReadProcessMemory(proc,(LPCVOID)a,&v,sizeof(T),0);return v;}
    template<typename T> void W(uintptr_t a,const T&v)const{WriteProcessMemory(proc,(LPVOID)a,&v,sizeof(T),0);}
    bool RB(uintptr_t a,void*d,size_t s)const{SIZE_T g=0;return ReadProcessMemory(proc,(LPCVOID)a,d,s,&g)&&g==s;}
    ~Mem(){if(proc)CloseHandle(proc);}
} g_mem;

std::string RS(uintptr_t a){
    if(!a)return{};
    size_t l=g_mem.R<size_t>(a+Off::STR_LEN);
    if(!l||l>512)return{};
    std::string r(l,'\0');
    if(l<16)g_mem.RB(a,r.data(),l);
    else{uintptr_t p=g_mem.R<uintptr_t>(a);if(!p)return{};g_mem.RB(p,r.data(),l);}
    return r;
}
std::string RCN(uintptr_t i){
    if(!i)return{};
    uintptr_t d=g_mem.R<uintptr_t>(i+Off::INST_DESC);if(!d)return{};
    uintptr_t n=g_mem.R<uintptr_t>(d+Off::INST_CNAME);if(!n)return{};
    char b[64]{};g_mem.RB(n,b,63);return b;
}
std::string RIN(uintptr_t i){
    if(!i)return{};
    uintptr_t nc=g_mem.R<uintptr_t>(i+Off::INST_NCONT);if(!nc)return{};
    return RS(nc+Off::INST_NAME);
}
std::vector<uintptr_t> GC(uintptr_t i){
    if(!i)return{};
    uintptr_t v=g_mem.R<uintptr_t>(i+Off::INST_CVEC);if(!v)return{};
    uintptr_t b=g_mem.R<uintptr_t>(v),e=g_mem.R<uintptr_t>(v+8);
    if(!b||e<=b)return{};
    size_t n=(e-b)/16;if(!n||n>8192)return{};
    std::vector<uintptr_t>ch;ch.reserve(n);
    for(size_t k=0;k<n;k++){uintptr_t c=g_mem.R<uintptr_t>(b+k*16);if(c)ch.push_back(c);}
    return ch;
}
uintptr_t FBC(uintptr_t p,const char*c){for(uintptr_t x:GC(p))if(x&&RCN(x)==c)return x;return 0;}
uintptr_t FBN(uintptr_t p,const char*n){for(uintptr_t x:GC(p))if(x&&RIN(x)==n)return x;return 0;}
Vec3 RPP(uintptr_t pt){uintptr_t pr=g_mem.R<uintptr_t>(pt+Off::BP_PRIM);if(!pr)return{};return g_mem.R<Vec3>(pr+Off::PRIM_POS);}

bool W2S(Vec3 wp,const VM&vm,int sw,int sh,Vec2&o){
    float x=vm.m[0][0]*wp.x+vm.m[0][1]*wp.y+vm.m[0][2]*wp.z+vm.m[0][3];
    float y=vm.m[1][0]*wp.x+vm.m[1][1]*wp.y+vm.m[1][2]*wp.z+vm.m[1][3];
    float w=vm.m[3][0]*wp.x+vm.m[3][1]*wp.y+vm.m[3][2]*wp.z+vm.m[3][3];
    if(w<0.001f)return false;
    o.x=(sw*.5f)*(1.f+x/w);o.y=(sh*.5f)*(1.f-y/w);return true;
}
float D3(Vec3 a,Vec3 b){float dx=a.x-b.x,dy=a.y-b.y,dz=a.z-b.z;return sqrtf(dx*dx+dy*dy+dz*dz);}

struct Entry{std::string name;Vec3 wp,cam;float hp,maxhp;Vec2 sf,sh;bool vis,onScr;uintptr_t ptr;};

HWND g_wnd=nullptr; int g_sw=0,g_sh=0;
std::atomic<bool> g_run{true};
std::vector<Entry> g_frame;
CRITICAL_SECTION g_cs;

// ImGui-style colors (GDI)
#define COL_BG    RGB(15,15,15)
#define COL_PANEL RGB(30,30,30)
#define COL_HDR   RGB(45,45,55)
#define COL_TITLE RGB(130,160,255)
#define COL_ON    RGB(80,200,80)
#define COL_OFF   RGB(200,60,60)
#define COL_TXT   RGB(220,220,220)
#define COL_DIM   RGB(100,100,100)

void GLine(HDC dc,int x1,int y1,int x2,int y2,COLORREF c,int t=1){
    HPEN p=CreatePen(PS_SOLID,t,c);HPEN o=(HPEN)SelectObject(dc,p);
    MoveToEx(dc,x1,y1,0);LineTo(dc,x2,y2);SelectObject(dc,o);DeleteObject(p);}
void GRect(HDC dc,int l,int t,int r,int b,COLORREF c){
    HBRUSH br=CreateSolidBrush(c);RECT rc{l,t,r,b};FillRect(dc,&rc,br);DeleteObject(br);}
void GBorder(HDC dc,int l,int t,int r,int b,COLORREF c){
    HPEN p=CreatePen(PS_SOLID,1,c);HPEN o=(HPEN)SelectObject(dc,p);
    SelectObject(dc,GetStockObject(NULL_BRUSH));Rectangle(dc,l,t,r,b);SelectObject(dc,o);DeleteObject(p);}
void GStr(HDC dc,int x,int y,const std::string&s,COLORREF c=COL_TXT){SetTextColor(dc,c);TextOutA(dc,x,y,s.c_str(),(int)s.size());}

void DrawCornerBox(HDC dc,int l,int t,int r,int b,COLORREF c){
    int cw=(r-l)/5,ch=(b-t)/5;
    HPEN p=CreatePen(PS_SOLID,1,c);SelectObject(dc,p);
    MoveToEx(dc,l,t+ch,0);LineTo(dc,l,t);LineTo(dc,l+cw,t);
    MoveToEx(dc,r-cw,t,0);LineTo(dc,r,t);LineTo(dc,r,t+ch);
    MoveToEx(dc,l,b-ch,0);LineTo(dc,l,b);LineTo(dc,l+cw,b);
    MoveToEx(dc,r-cw,b,0);LineTo(dc,r,b);LineTo(dc,r,b-ch);
    DeleteObject(p);}
void DrawHPBar(HDC dc,int x,int t,int b,float f){
    f=std::max(0.f,std::min(1.f,f));int fy=b-(int)((b-t)*f);
    HPEN bg=CreatePen(PS_SOLID,2,RGB(15,15,15));
    HPEN bar=CreatePen(PS_SOLID,2,RGB((int)(255*(1-f)),(int)(210*f),25));
    SelectObject(dc,bg);MoveToEx(dc,x,t,0);LineTo(dc,x,b);
    SelectObject(dc,bar);MoveToEx(dc,x,fy,0);LineTo(dc,x,b);
    DeleteObject(bg);DeleteObject(bar);}

void DrawEdgeArrow(HDC dc,Vec2 sp,int sw,int sh,COLORREF col){
    float cx=sw*.5f,cy=sh*.5f,dx=sp.x-cx,dy=sp.y-cy;
    float len=sqrtf(dx*dx+dy*dy);if(len<1)return;dx/=len;dy/=len;
    float mg=18.f,tx=cx,ty=cy,tmax=1e9f;
    if(dx>0)tmax=std::min(tmax,(sw-mg-cx)/dx);else if(dx<0)tmax=std::min(tmax,(mg-cx)/dx);
    if(dy>0)tmax=std::min(tmax,(sh-mg-cy)/dy);else if(dy<0)tmax=std::min(tmax,(mg-cy)/dy);
    tx=cx+dx*tmax;ty=cy+dy*tmax;
    float ax=-dy*5,ay=dx*5;
    POINT pts[3]={{(LONG)(tx+dx*10),(LONG)(ty+dy*10)},{(LONG)(tx+ax),(LONG)(ty+ay)},{(LONG)(tx-ax),(LONG)(ty-ay)}};
    HBRUSH br=CreateSolidBrush(col);HPEN pe=CreatePen(PS_SOLID,1,col);
    SelectObject(dc,br);SelectObject(dc,pe);Polygon(dc,pts,3);
    DeleteObject(br);DeleteObject(pe);}

// ImGui-style menu
void DrawMenu(HDC dc){
    int mx=12,my=12,mw=240;
    auto Row=[&](int r,const char*key,const char*lbl,bool on){
        int ty=my+34+r*26;
        GRect(dc,mx,ty,mx+mw,ty+22,(r%2==0)?RGB(32,32,38):RGB(28,28,34));
        char buf[64];sprintf(buf,"%-5s %s",key,lbl);
        GStr(dc,mx+8,ty+4,buf,on?COL_TXT:COL_DIM);
        GRect(dc,mx+mw-22,ty+5,mx+mw-8,ty+17,on?COL_ON:COL_OFF);
    };
    int rows=5;int mh=38+rows*26+8;
    GRect(dc,mx,my,mx+mw,my+mh,COL_PANEL);
    GRect(dc,mx,my,mx+mw,my+26,COL_HDR);
    GBorder(dc,mx,my,mx+mw,my+mh,RGB(60,60,80));
    GStr(dc,mx+8,my+6,"  ESP MENU",COL_TITLE);
    Row(0,"[F1]","Boxes",G.boxes.load());
    Row(1,"[F2]","Tracers",G.tracers.load());
    Row(2,"[F3]","HP Bars",G.hp.load());
    Row(3,"[F4]","Names",G.names.load());
    Row(4,"[F5]","Fly",G.fly.load());
    GStr(dc,mx+8,my+mh-16,"[INS] Close  [LALT] Aimbot",COL_DIM);}

LRESULT CALLBACK WP(HWND hw,UINT msg,WPARAM wp,LPARAM lp){
    if(msg==WM_DESTROY){g_run=false;PostQuitMessage(0);return 0;}
    if(msg!=WM_PAINT)return DefWindowProcW(hw,msg,wp,lp);
    PAINTSTRUCT ps;HDC hdc=BeginPaint(hw,&ps);
    HDC mdc=CreateCompatibleDC(hdc);HBITMAP bmp=CreateCompatibleBitmap(hdc,g_sw,g_sh);
    SelectObject(mdc,bmp);
    RECT rc{0,0,g_sw,g_sh};FillRect(mdc,&rc,(HBRUSH)GetStockObject(BLACK_BRUSH));
    HFONT fnt=CreateFontA(11,0,0,0,FW_BOLD,0,0,0,ANSI_CHARSET,0,0,CLEARTYPE_QUALITY,0,"Tahoma");
    SelectObject(mdc,fnt);SetBkMode(mdc,TRANSPARENT);
    EnterCriticalSection(&g_cs);auto snap=g_frame;LeaveCriticalSection(&g_cs);
    int cx=g_sw/2,cy=g_sh/2;
    for(auto&e:snap){
        if(!e.vis)continue;
        float frac=e.maxhp>0?e.hp/e.maxhp:0.f;
        COLORREF col=RGB(255,(int)(55*frac),(int)(55*frac));
        if(!e.onScr){DrawEdgeArrow(mdc,e.sf,g_sw,g_sh,col);continue;}
        int fx=(int)e.sf.x,fy=(int)e.sf.y,hy=(int)e.sh.y;
        float bh=fabsf((float)(fy-hy));if(bh<4)continue;
        float bw=bh*0.45f;
        int l=(int)(fx-bw*.5f),r=(int)(fx+bw*.5f),t=hy-2,b=fy;
        if(G.tracers.load())GLine(mdc,cx,cy,fx,fy,RGB(255,200,50));
        if(G.boxes.load())DrawCornerBox(mdc,l,t,r,b,col);
        if(G.hp.load())DrawHPBar(mdc,l-5,t,b,frac);
        if(G.names.load()){
            std::ostringstream s;s<<e.name<<" ["<<(int)(D3(e.wp,e.cam)*0.0284f)<<"m]";
            GStr(mdc,l,t-14,s.str(),RGB(255,232,170));
            std::ostringstream h;h<<(int)e.hp<<"/"<<(int)e.maxhp;
            GStr(mdc,l,b+2,h.str(),COL_ON);}}
    if(G.menu.load())DrawMenu(mdc);
    DeleteObject(fnt);BitBlt(hdc,0,0,g_sw,g_sh,mdc,0,0,SRCCOPY);
    DeleteObject(bmp);DeleteDC(mdc);EndPaint(hw,&ps);return 0;}

void MakeOverlay(){
    g_sw=GetSystemMetrics(SM_CXSCREEN);g_sh=GetSystemMetrics(SM_CYSCREEN);
    WNDCLASSEXW wc{sizeof(wc)};wc.lpfnWndProc=WP;wc.hInstance=GetModuleHandleW(0);wc.lpszClassName=L"RESP25";
    RegisterClassExW(&wc);
    g_wnd=CreateWindowExW(WS_EX_TOPMOST|WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_TOOLWINDOW,L"RESP25",L" ",WS_POPUP,0,0,g_sw,g_sh,0,0,wc.hInstance,0);
    SetLayeredWindowAttributes(g_wnd,RGB(0,0,0),0,LWA_COLORKEY);ShowWindow(g_wnd,SW_SHOW);}

static float g_ax=0,g_ay=0;
void DoAimbot(const std::vector<Entry>&f,Vec3 lpos){
    if(!(GetAsyncKeyState(VK_LMENU)&0x8000)){g_ax=g_ay=0;return;}
    float best=9e9f;Vec3 btgt{};bool found=false;
    for(auto&e:f){
        if(!e.vis)continue;
        float d=D3(e.wp,lpos);
        if(d<best){best=d;btgt={e.wp.x,e.wp.y+3.f,e.wp.z};found=true;}}
    if(!found)return;
    Vec2 sc; VM vm2{};// reproject using current frame - approximate via snap
    // We project btgt to screen using the latest frame data
    // Use e.sh of closest enemy
    for(auto&e:f){if(!e.vis)continue;if(D3(e.wp,lpos)<best+0.1f){sc=e.sh;break;}}
    Vec2 center{(float)g_sw*.5f,(float)g_sh*.5f};
    g_ax+=(sc.x-center.x)/5.f;g_ay+=(sc.y-center.y)/5.f;
    LONG mx=(LONG)g_ax,my=(LONG)g_ay;g_ax-=mx;g_ay-=my;
    if(mx||my){INPUT in{};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_MOVE;in.mi.dx=mx;in.mi.dy=my;SendInput(1,&in,sizeof(in));}}

void Logic(){
    bool pI=0,pF1=0,pF2=0,pF3=0,pF4=0,pF5=0;
    static uintptr_t localHum=0;
    auto P=[](int k){return(GetAsyncKeyState(k)&0x8000)!=0;};
    while(g_run){
        bool cI=P(VK_INSERT);if(cI&&!pI)G.menu=!G.menu.load();pI=cI;
        bool c1=P(VK_F1);if(c1&&!pF1)G.boxes=!G.boxes.load();pF1=c1;
        bool c2=P(VK_F2);if(c2&&!pF2)G.tracers=!G.tracers.load();pF2=c2;
        bool c3=P(VK_F3);if(c3&&!pF3)G.hp=!G.hp.load();pF3=c3;
        bool c4=P(VK_F4);if(c4&&!pF4)G.names=!G.names.load();pF4=c4;
        bool c5=P(VK_F5);if(c5&&!pF5)G.fly=!G.fly.load();pF5=c5;

        uintptr_t fakeDm=g_mem.R<uintptr_t>(g_mem.base+Off::FakeDM_Ptr);
        uintptr_t dm=fakeDm?g_mem.R<uintptr_t>(fakeDm+Off::FakeDM_RealDM):0;
        if(!dm){Sleep(200);continue;}

        VM vm{};
        uintptr_t ve=g_mem.R<uintptr_t>(g_mem.base+Off::VE_Ptr);
        if(ve)vm=g_mem.R<VM>(ve+Off::VE_VM);
        if(vm.m[3][3]==0.f){Sleep(16);continue;}

        Vec3 cam{};
        uintptr_t ws=g_mem.R<uintptr_t>(dm+Off::DM_WS);
        uintptr_t cam_inst=ws?g_mem.R<uintptr_t>(ws+Off::WS_CAM):0;
        if(cam_inst)cam=g_mem.R<Vec3>(cam_inst+Off::CAM_POS);

        uintptr_t plySvc=FBN(dm,"Players");
        uintptr_t localPl=plySvc?g_mem.R<uintptr_t>(plySvc+Off::PL_LOCAL):0;
        uintptr_t localHRP=0;
        uintptr_t localTeam=0;

        if(localPl){
            localTeam=g_mem.R<uintptr_t>(localPl+Off::PL_TEAM);
            uintptr_t lc=g_mem.R<uintptr_t>(localPl+Off::PL_MODEL);
            if(lc){
                uintptr_t lh=FBC(lc,"Humanoid");if(!lh)lh=FBN(lc,"Humanoid");
                if(lh){localHum=lh;localHRP=g_mem.R<uintptr_t>(lh+Off::HUM_HRP);}}}

        // Fly (IY-style velocity)
        if(G.fly.load()&&localHRP&&localHum){
            uintptr_t pr=g_mem.R<uintptr_t>(localHRP+Off::BP_PRIM);
            if(pr){
                g_mem.W<uint8_t>(localHum+0x1dc,1);
                float fx=-vm.m[2][0],fz=-vm.m[2][2],rx=vm.m[0][0],rz=vm.m[0][2];
                float fl=sqrtf(fx*fx+fz*fz),rl=sqrtf(rx*rx+rz*rz);
                if(fl>0.001f){fx/=fl;fz/=fl;}if(rl>0.001f){rx/=rl;rz/=rl;}
                const float spd=50.f;float vx=0,vy=0,vz=0;
                if(P('W')){vx+=fx*spd;vz+=fz*spd;}if(P('S')){vx-=fx*spd;vz-=fz*spd;}
                if(P('A')){vx-=rx*spd;vz-=rz*spd;}if(P('D')){vx+=rx*spd;vz+=rz*spd;}
                if(P(VK_SPACE))vy=spd;if(P(VK_LSHIFT))vy=-spd;
                g_mem.W<Vec3>(pr+Off::PRIM_VEL,{vx,vy,vz});
                g_mem.W<Vec3>(pr+0x104,{0,0,0});}}
        else if(localHum&&!G.fly.load())g_mem.W<uint8_t>(localHum+0x1dc,0);

        // ESP
        std::vector<Entry> frame;
        if(plySvc){
            for(uintptr_t pl:GC(plySvc)){
                if(!pl||pl==localPl)continue;
                if(RCN(pl)!="Player")continue;
                // Team filter: if local has team, skip same team
                uintptr_t enemyTeam=g_mem.R<uintptr_t>(pl+Off::PL_TEAM);
                if(localTeam&&localTeam==enemyTeam)continue;
                Entry e{};e.name=RIN(pl);e.cam=cam;e.ptr=pl;
                // Player selection filter
                if(!G.selectedPlayers.empty()&&G.selectedPlayers.find(e.name)==G.selectedPlayers.end())continue;
                uintptr_t chr=g_mem.R<uintptr_t>(pl+Off::PL_MODEL);if(!chr)continue;
                uintptr_t hum=FBC(chr,"Humanoid");if(!hum)hum=FBN(chr,"Humanoid");if(!hum)continue;
                e.hp=g_mem.R<float>(hum+Off::HUM_HP);e.maxhp=g_mem.R<float>(hum+Off::HUM_MAXHP);
                if(e.hp<=0.f)continue;
                uintptr_t hrp=g_mem.R<uintptr_t>(hum+Off::HUM_HRP);if(!hrp)continue;
                e.wp=RPP(hrp);
                Vec3 hd{e.wp.x,e.wp.y+3.f,e.wp.z};
                bool of=W2S(e.wp,vm,g_sw,g_sh,e.sf),oh=W2S(hd,vm,g_sw,g_sh,e.sh);
                e.vis=of&&oh;
                e.onScr=(e.sf.x>-100&&e.sf.x<g_sw+100&&e.sf.y>-100&&e.sf.y<g_sh+100);
                frame.push_back(std::move(e));}}

        // Aimbot: nearest enemy in 3D
        DoAimbot(frame,cam);

        EnterCriticalSection(&g_cs);g_frame=std::move(frame);LeaveCriticalSection(&g_cs);
        SetWindowPos(g_wnd,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
        InvalidateRect(g_wnd,0,FALSE);UpdateWindow(g_wnd);Sleep(8);}}

int WINAPI WinMain(HINSTANCE,HINSTANCE,LPSTR,int){
    InitializeCriticalSection(&g_cs);
    while(!g_mem.Attach(L"RobloxPlayerBeta.exe"))
        if(MessageBoxW(0,L"Roblox introuvable — réessayer?",L"ESP",MB_RETRYCANCEL)==IDCANCEL)return 0;
    MakeOverlay();std::thread t(Logic);
    MSG msg{};while(g_run&&GetMessageW(&msg,0,0,0)){TranslateMessage(&msg);DispatchMessageW(&msg);}
    g_run=false;t.join();DeleteCriticalSection(&g_cs);return 0;}
