#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <deque>
#include <fstream>

const int CELL=28,COLS=10,ROWS=20,LEFT_W=170,RIGHT_W=160,TOP_H=40,BOT_H=80;
const int BX=LEFT_W,BY=TOP_H;
const int WIN_W=LEFT_W+COLS*CELL+RIGHT_W,WIN_H=TOP_H+ROWS*CELL+BOT_H;

struct Col{Uint8 r,g,b,a;};
const Col PCOL[]={{0,0,0,0},{0,220,220,255},{220,220,0,255},{150,0,220,255},
    {0,220,0,255},{220,0,0,255},{0,100,220,255},{220,140,0,255}};
const Col C_BG={10,12,28,255},C_PANEL={18,22,42,255},C_BOR={0,180,220,255};
const Col C_TEXT={200,220,240,255},C_DIM={80,100,130,255};
const Col C_TITLE={0,200,255,255},C_ACC={0,160,200,255},C_RED={255,60,60,255};

struct Pt{int x,y;};
const Pt SH[7][4][4]={
    {{{0,1},{1,1},{2,1},{3,1}},{{2,0},{2,1},{2,2},{2,3}},{{0,2},{1,2},{2,2},{3,2}},{{1,0},{1,1},{1,2},{1,3}}},
    {{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}},{{1,0},{2,0},{1,1},{2,1}}},
    {{{1,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{2,1},{1,2}},{{1,0},{0,1},{1,1},{1,2}}},
    {{{1,0},{2,0},{0,1},{1,1}},{{1,0},{1,1},{2,1},{2,2}},{{1,1},{2,1},{0,2},{1,2}},{{0,0},{0,1},{1,1},{1,2}}},
    {{{0,0},{1,0},{1,1},{2,1}},{{2,0},{1,1},{2,1},{1,2}},{{0,1},{1,1},{1,2},{2,2}},{{1,0},{0,1},{1,1},{0,2}}},
    {{{0,0},{0,1},{1,1},{2,1}},{{1,0},{2,0},{1,1},{1,2}},{{0,1},{1,1},{2,1},{2,2}},{{1,0},{1,1},{0,2},{1,2}}},
    {{{2,0},{0,1},{1,1},{2,1}},{{1,0},{1,1},{1,2},{2,2}},{{0,1},{1,1},{2,1},{0,2}},{{0,0},{1,0},{1,1},{1,2}}},
};

// 7-Bag Randomizer
struct Bag{
    std::vector<int> b;
    void fill(){b={0,1,2,3,4,5,6};for(int i=6;i>0;i--)std::swap(b[i],b[rand()%(i+1)]);}
    int next(){if(b.empty())fill();int v=b.back();b.pop_back();return v;}
    Bag(){fill();}
} bag;

int board[ROWS][COLS]={};
struct Piece{int type,rot,x,y;};
Piece cur,ghost;
std::deque<int> nxtQ;
int holdType=-1;
bool holdUsed=false;
int score=0,lines=0,level=1,best=0;

// Leaderboard - sorted vector (insertion sort)
std::vector<int> lb;
void addScore(int s){
    lb.push_back(s);
    for(int i=(int)lb.size()-1;i>0&&lb[i]>lb[i-1];i--) std::swap(lb[i],lb[i-1]);
    if((int)lb.size()>5) lb.resize(5);
    if(!lb.empty()) best=lb[0];
}

// Game State Machine
enum GState{GS_PLAY,GS_PAUSE,GS_OVER};
GState gstate=GS_PLAY;

bool drag=false;
int dragOX=0,dragOY=0;

// DAS Input
const int DAS_D=170,DAS_R=50;
struct DasKey{bool held=false;Uint32 t0=0,tr=0;};
DasKey dL,dR,dD;
Uint32 lastDrop=0;
int dropMs(){return SDL_max(50,500-(level-1)*45);}

// Collision Detection
bool fits(int t,int r,int x,int y){
    for(int i=0;i<4;i++){
        int nx=x+SH[t][r][i].x,ny=y+SH[t][r][i].y;
        if(nx<0||nx>=COLS||ny<0||ny>=ROWS) return false;
        if(ny>=0&&board[ny][nx]) return false;
    }
    return true;
}

// Greedy Drop - Ghost Piece
void updGhost(){ghost=cur;while(fits(ghost.type,ghost.rot,ghost.x,ghost.y+1))ghost.y++;}

void lockPiece(){
    for(int i=0;i<4;i++){
        int nx=cur.x+SH[cur.type][cur.rot][i].x;
        int ny=cur.y+SH[cur.type][cur.rot][i].y;
        if(ny>=0) board[ny][nx]=cur.type+1;
    }
}

// Line Clearing Algorithm
int clearLines(){
    int cl=0;
    for(int r=ROWS-1;r>=0;r--){
        bool full=true;
        for(int c=0;c<COLS;c++) if(!board[r][c]){full=false;break;}
        if(full){
            for(int rr=r;rr>0;rr--) for(int c=0;c<COLS;c++) board[rr][c]=board[rr-1][c];
            for(int c=0;c<COLS;c++) board[0][c]=0;
            cl++;r++;
        }
    }
    return cl;
}

void afterLock(){
    int cl=clearLines();
    if(cl){const int pts[]={0,100,300,500,800};score+=pts[cl]*level;lines+=cl;level=lines/10+1;}
    if(score>best) best=score;
}

void spawn(int type){
    cur={type,0,COLS/2-2,0};holdUsed=false;updGhost();
    if(!fits(cur.type,cur.rot,cur.x,cur.y)){gstate=GS_OVER;addScore(score);}
}

void spawnNext(){
    while((int)nxtQ.size()<3) nxtQ.push_back(bag.next());
    int t=nxtQ.front();nxtQ.pop_front();nxtQ.push_back(bag.next());
    spawn(t);
}

void doHold(){
    if(holdUsed) return;holdUsed=true;
    if(holdType<0){holdType=cur.type;spawnNext();holdUsed=true;}
    else{int tmp=holdType;holdType=cur.type;spawn(tmp);holdUsed=true;}
}

void hardDrop(){
    updGhost();score+=(ghost.y-cur.y)*2;cur=ghost;
    lockPiece();afterLock();spawnNext();
}

void tryRotate(int dir){
    int nr=(cur.rot+dir+4)%4;
    if(fits(cur.type,nr,cur.x,cur.y)){cur.rot=nr;updGhost();}
    else if(fits(cur.type,nr,cur.x-1,cur.y)){cur.x--;cur.rot=nr;updGhost();}
    else if(fits(cur.type,nr,cur.x+1,cur.y)){cur.x++;cur.rot=nr;updGhost();}
}

void resetGame(){
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++) board[r][c]=0;
    score=lines=0;level=1;holdType=-1;holdUsed=false;
    nxtQ.clear();bag=Bag();
    while((int)nxtQ.size()<3) nxtQ.push_back(bag.next());
    gstate=GS_PLAY;spawnNext();lastDrop=SDL_GetTicks();
}

SDL_Renderer* ren;
TTF_Font *fSm,*fMd,*fLg;

void sc(Col c){SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,c.a);}
void fr(int x,int y,int w,int h,Col c){sc(c);SDL_Rect r={x,y,w,h};SDL_RenderFillRect(ren,&r);}
void dr(int x,int y,int w,int h,Col c){sc(c);SDL_Rect r={x,y,w,h};SDL_RenderDrawRect(ren,&r);}

void drawCell(int px,int py,int ci,int sz=CELL,Uint8 a=255){
    Col c=PCOL[ci];
    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren,c.r,c.g,c.b,a);
    SDL_Rect r={px+1,py+1,sz-2,sz-2};SDL_RenderFillRect(ren,&r);
    Col hi={(Uint8)SDL_min(c.r+60,255),(Uint8)SDL_min(c.g+60,255),(Uint8)SDL_min(c.b+60,255),a};
    SDL_SetRenderDrawColor(ren,hi.r,hi.g,hi.b,hi.a);
    SDL_Rect t2={px+1,py+1,sz-2,2},l2={px+1,py+1,2,sz-2};
    SDL_RenderFillRect(ren,&t2);SDL_RenderFillRect(ren,&l2);
}

void txt(TTF_Font*f,const std::string&s,int x,int y,Col c={200,220,240,255},bool cen=false){
    SDL_Color sc2={c.r,c.g,c.b,255};
    SDL_Surface*su=TTF_RenderText_Blended(f,s.c_str(),sc2);if(!su)return;
    SDL_Texture*t=SDL_CreateTextureFromSurface(ren,su);
    SDL_Rect d={cen?x-su->w/2:x,y,su->w,su->h};
    SDL_FreeSurface(su);SDL_RenderCopy(ren,t,nullptr,&d);SDL_DestroyTexture(t);
}

void prevPiece(int type,int cx,int cy,int sz=14){
    for(int i=0;i<4;i++) drawCell(cx+SH[type][0][i].x*sz,cy+SH[type][0][i].y*sz,type+1,sz);
}

void doRender(){
    sc(C_BG);SDL_RenderClear(ren);

    // Top bar
    fr(0,0,WIN_W,TOP_H,C_PANEL);
    fr(5,6,75,28,C_ACC);txt(fSm,"LEVEL "+std::to_string(level),12,13,{255,255,255,255});
    txt(fLg,"TETRIS",WIN_W/2,4,C_TITLE,true);
    txt(fSm,"BEST",WIN_W-65,7,C_DIM);
    txt(fMd,std::to_string(best),WIN_W-65,20,C_TEXT);

    // Left panel
    fr(0,TOP_H,LEFT_W,ROWS*CELL,C_PANEL);
    dr(0,TOP_H,LEFT_W,ROWS*CELL,{25,35,65,255});
    int ly=TOP_H+8;

    fr(5,ly,LEFT_W-10,52,{12,15,35,255});dr(5,ly,LEFT_W-10,52,{30,40,80,255});
    txt(fSm,"SCORE",LEFT_W/2,ly+4,C_DIM,true);
    txt(fLg,std::to_string(score),LEFT_W/2,ly+18,C_TEXT,true);ly+=58;

    txt(fSm,"LINES",8,ly,C_DIM);txt(fSm,std::to_string(lines),LEFT_W-30,ly,C_TEXT);ly+=15;
    fr(8,ly,LEFT_W-16,5,{20,25,50,255});
    fr(8,ly,(int)((LEFT_W-16)*(lines%10)/10.0f),5,C_ACC);ly+=10;
    txt(fSm,std::to_string(10-lines%10)+" to L"+std::to_string(level+1),LEFT_W/2,ly,C_DIM,true);ly+=16;

    txt(fSm,"HOLD",LEFT_W/2,ly,C_DIM,true);ly+=13;
    fr(8,ly,LEFT_W-16,50,{12,15,35,255});dr(8,ly,LEFT_W-16,50,{30,40,80,255});
    if(holdType>=0) prevPiece(holdType,14,ly+8,14);
    else txt(fSm,"C to hold",LEFT_W/2,ly+18,C_DIM,true);
    ly+=56;

    txt(fSm,"SPEED",8,ly,C_DIM);ly+=13;
    fr(8,ly,LEFT_W-16,5,{20,25,50,255});
    fr(8,ly,(int)((LEFT_W-16)*SDL_min(1.0f,(level-1)/10.0f)),5,C_ACC);ly+=12;

    fr(5,ly,LEFT_W-10,1,{30,40,80,255});ly+=5;
    txt(fSm,"CONTROLS",LEFT_W/2,ly,C_DIM,true);ly+=13;
    struct CT{const char*k,*d;};
    CT ctrls[]=
    {{"<- ->","Move"},{"^ / X","Rotate CW"},{"Z","Rotate CCW"},
     {"v","Soft drop"},{"SPC","Hard drop"},{"C","Hold"},{"P","Pause"},{"Drag","Drop piece"}};
    for(auto&ct:ctrls){txt(fSm,ct.k,8,ly,C_ACC);txt(fSm,ct.d,62,ly,C_DIM);ly+=12;}

    // Board
    fr(BX,BY,COLS*CELL,ROWS*CELL,{8,10,22,255});
    for(int r=0;r<ROWS;r++) for(int c=0;c<COLS;c++)
        if(board[r][c]) drawCell(BX+c*CELL,BY+r*CELL,board[r][c]);

    // Ghost piece
    if(gstate==GS_PLAY){
        for(int i=0;i<4;i++){
            int px=BX+(ghost.x+SH[ghost.type][ghost.rot][i].x)*CELL;
            int py=BY+(ghost.y+SH[ghost.type][ghost.rot][i].y)*CELL;
            Col gc=PCOL[ghost.type+1];
            SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren,gc.r,gc.g,gc.b,55);
            SDL_Rect gr={px+1,py+1,CELL-2,CELL-2};SDL_RenderFillRect(ren,&gr);
            SDL_SetRenderDrawColor(ren,gc.r,gc.g,gc.b,110);
            SDL_RenderDrawRect(ren,&gr);
        }
    }

    if(gstate==GS_PLAY&&!drag)
        for(int i=0;i<4;i++)
            drawCell(BX+(cur.x+SH[cur.type][cur.rot][i].x)*CELL,
                     BY+(cur.y+SH[cur.type][cur.rot][i].y)*CELL,cur.type+1);

    SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren,30,40,70,70);
    for(int c=0;c<=COLS;c++) SDL_RenderDrawLine(ren,BX+c*CELL,BY,BX+c*CELL,BY+ROWS*CELL);
    for(int r=0;r<=ROWS;r++) SDL_RenderDrawLine(ren,BX,BY+r*CELL,BX+COLS*CELL,BY+r*CELL);
    dr(BX-1,BY-1,COLS*CELL+2,ROWS*CELL+2,C_BOR);

    // Right panel
    int rx=BX+COLS*CELL,ry=TOP_H+8;
    fr(rx,TOP_H,RIGHT_W,ROWS*CELL,C_PANEL);
    dr(rx,TOP_H,RIGHT_W,ROWS*CELL,{25,35,65,255});

    txt(fSm,"NEXT",rx+RIGHT_W/2,ry,C_DIM,true);ry+=14;
    for(int n=0;n<3&&n<(int)nxtQ.size();n++){
        fr(rx+5,ry,RIGHT_W-10,46,{12,15,35,255});dr(rx+5,ry,RIGHT_W-10,46,{25,35,70,255});
        prevPiece(nxtQ[n],rx+10,ry+6,13);ry+=52;
    }
    ry+=6;

    fr(rx,ry,RIGHT_W,1,{30,40,80,255});ry+=5;
    txt(fSm,"LEADERBOARD",rx+RIGHT_W/2,ry,C_DIM,true);ry+=13;
    for(int i=0;i<(int)lb.size()&&i<5;i++){
        Col nc=(i==0)?C_TITLE:(i==1)?Col{180,220,240,255}:C_DIM;
        txt(fSm,"#"+std::to_string(i+1),rx+6,ry,nc);
        std::string ss=std::to_string(lb[i]);
        txt(fSm,ss,rx+RIGHT_W-8-(int)ss.size()*7,ry,nc);
        ry+=13;
    }
    ry+=6;
    fr(rx,ry,RIGHT_W,1,{30,40,80,255});ry+=5;
    txt(fSm,"THIS GAME",rx+RIGHT_W/2,ry,C_DIM,true);ry+=13;
    txt(fSm,"Score:",rx+6,ry,C_DIM);txt(fSm,std::to_string(score),rx+75,ry,C_TEXT);ry+=13;
    txt(fSm,"Lines:",rx+6,ry,C_DIM);txt(fSm,std::to_string(lines),rx+75,ry,C_TEXT);ry+=13;
    txt(fSm,"Level:",rx+6,ry,C_DIM);txt(fSm,std::to_string(level),rx+75,ry,C_TEXT);

    // Bottom bar
    int bY=TOP_H+ROWS*CELL;
    fr(0,bY,WIN_W,BOT_H,C_PANEL);fr(0,bY,WIN_W,1,C_BOR);
    txt(fSm,"DRAG TO PLACE  *  ACTIVE PIECE",WIN_W/2,bY+5,C_DIM,true);
    int bpx=WIN_W/2-50;
    for(int i=0;i<4;i++) drawCell(bpx+SH[cur.type][0][i].x*18,bY+18+SH[cur.type][0][i].y*18,cur.type+1,18);
    txt(fSm,"hold & drag -> NEXT:",WIN_W/2+15,bY+22,C_DIM);
    int npx=WIN_W/2+138;
    for(int n=0;n<2&&n<(int)nxtQ.size();n++){
        for(int i=0;i<4;i++) drawCell(npx+SH[nxtQ[n]][0][i].x*10,bY+20+SH[nxtQ[n]][0][i].y*10,nxtQ[n]+1,10);
        npx+=48;
    }

    if(drag){
        int mx,my;SDL_GetMouseState(&mx,&my);
        for(int i=0;i<4;i++)
            drawCell(mx+dragOX+SH[cur.type][cur.rot][i].x*CELL,
                     my+dragOY+SH[cur.type][cur.rot][i].y*CELL,cur.type+1,CELL,160);
    }

    if(gstate==GS_PAUSE){
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren,0,0,0,160);
        SDL_Rect ov={BX,BY,COLS*CELL,ROWS*CELL};SDL_RenderFillRect(ren,&ov);
        txt(fLg,"PAUSED",BX+COLS*CELL/2,BY+ROWS*CELL/2-20,C_TITLE,true);
        txt(fSm,"Press P to resume",BX+COLS*CELL/2,BY+ROWS*CELL/2+15,C_DIM,true);
    }
    if(gstate==GS_OVER){
        SDL_SetRenderDrawBlendMode(ren,SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren,0,0,0,175);
        SDL_Rect ov={BX,BY,COLS*CELL,ROWS*CELL};SDL_RenderFillRect(ren,&ov);
        txt(fLg,"GAME OVER",BX+COLS*CELL/2,BY+ROWS*CELL/2-35,C_RED,true);
        txt(fMd,"Score: "+std::to_string(score),BX+COLS*CELL/2,BY+ROWS*CELL/2+5,C_TEXT,true);
        txt(fSm,"Press R to restart",BX+COLS*CELL/2,BY+ROWS*CELL/2+35,C_DIM,true);
    }
    SDL_RenderPresent(ren);
}

void handleDAS(Uint32 now){
    if(gstate!=GS_PLAY||drag) return;
    const Uint8*k=SDL_GetKeyboardState(nullptr);
    if(k[SDL_SCANCODE_LEFT]){
        if(!dL.held){dL.held=true;dL.t0=now;dL.tr=now;if(fits(cur.type,cur.rot,cur.x-1,cur.y)){cur.x--;updGhost();}}
        else if(now-dL.t0>=(Uint32)DAS_D&&now-dL.tr>=(Uint32)DAS_R){dL.tr=now;if(fits(cur.type,cur.rot,cur.x-1,cur.y)){cur.x--;updGhost();}}
    } else dL.held=false;
    if(k[SDL_SCANCODE_RIGHT]){
        if(!dR.held){dR.held=true;dR.t0=now;dR.tr=now;if(fits(cur.type,cur.rot,cur.x+1,cur.y)){cur.x++;updGhost();}}
        else if(now-dR.t0>=(Uint32)DAS_D&&now-dR.tr>=(Uint32)DAS_R){dR.tr=now;if(fits(cur.type,cur.rot,cur.x+1,cur.y)){cur.x++;updGhost();}}
    } else dR.held=false;
    if(k[SDL_SCANCODE_DOWN]){
        if(!dD.held){dD.held=true;dD.t0=now;dD.tr=now;if(fits(cur.type,cur.rot,cur.x,cur.y+1)){cur.y++;score++;}}
        else if(now-dD.t0>=(Uint32)DAS_D&&now-dD.tr>=(Uint32)DAS_R){dD.tr=now;if(fits(cur.type,cur.rot,cur.x,cur.y+1)){cur.y++;score++;}}
    } else dD.held=false;
}

int main(int argc,char*argv[]){
    srand((unsigned)time(nullptr));
    if(SDL_Init(SDL_INIT_VIDEO)<0){std::cout<<SDL_GetError();return 1;}
    if(TTF_Init()<0){std::cout<<TTF_GetError();return 1;}
    SDL_Window*win=SDL_CreateWindow("Tetris - Enhanced Edition",
        SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,WIN_W,WIN_H,0);
    ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);
    fSm=TTF_OpenFont("font.ttf",12);fMd=TTF_OpenFont("font.ttf",18);fLg=TTF_OpenFont("font.ttf",26);
    if(!fSm||!fMd||!fLg){std::cout<<"Put font.ttf in game folder!\n";return 1;}

    {std::ifstream f("scores.dat");int s;
     while(f>>s) lb.push_back(s);
     std::sort(lb.begin(),lb.end(),std::greater<int>());
     if(!lb.empty()) best=lb[0];}

    resetGame();
    bool running=true;SDL_Event e;

    while(running){
        Uint32 now=SDL_GetTicks();
        while(SDL_PollEvent(&e)){
            if(e.type==SDL_QUIT) running=false;
            if(e.type==SDL_KEYDOWN){
                if(gstate==GS_OVER){if(e.key.keysym.sym==SDLK_r)resetGame();continue;}
                if(e.key.keysym.sym==SDLK_p){gstate=(gstate==GS_PAUSE)?GS_PLAY:GS_PAUSE;continue;}
                if(gstate==GS_PAUSE) continue;
                switch(e.key.keysym.sym){
                    case SDLK_UP: case SDLK_x: tryRotate(1);break;
                    case SDLK_z: tryRotate(-1);break;
                    case SDLK_SPACE: hardDrop();break;
                    case SDLK_c: doHold();break;
                    default:break;
                }
            }
            // Drag & Drop - Geometry Mapping (mouse pos -> board coordinates)
            if(e.type==SDL_MOUSEBUTTONDOWN&&e.button.button==SDL_BUTTON_LEFT&&gstate==GS_PLAY){
                int mx=e.button.x,my=e.button.y;
                bool found=false;
                for(int i=0;i<4&&!found;i++){
                    int px=BX+(cur.x+SH[cur.type][cur.rot][i].x)*CELL;
                    int py=BY+(cur.y+SH[cur.type][cur.rot][i].y)*CELL;
                    if(mx>=px&&mx<px+CELL&&my>=py&&my<py+CELL){
                        drag=true;dragOX=-SH[cur.type][cur.rot][i].x*CELL;
                        dragOY=-SH[cur.type][cur.rot][i].y*CELL;found=true;
                    }
                }
                if(!found){
                    int bpx=WIN_W/2-50,bY2=TOP_H+ROWS*CELL;
                    if(my>=bY2+15&&my<=bY2+60&&mx>=bpx&&mx<=bpx+80){drag=true;dragOX=0;dragOY=0;}
                }
            }
            if(e.type==SDL_MOUSEBUTTONUP&&e.button.button==SDL_BUTTON_LEFT&&drag){
                drag=false;
                int mx=e.button.x,my=e.button.y;
                int bc=(mx-BX)/CELL,br=(my-BY)/CELL;
                bool placed=false;
                for(int dy=0;dy<=2&&!placed;dy++)
                    for(int dx=-2;dx<=2&&!placed;dx++)
                        if(fits(cur.type,cur.rot,bc-1+dx,br-1+dy)){
                            cur.x=bc-1+dx;cur.y=br-1+dy;placed=true;
                        }
                updGhost();
            }
        }
        handleDAS(now);
        if(gstate==GS_PLAY&&!drag){
            if(now-lastDrop>=(Uint32)dropMs()){
                lastDrop=now;
                if(fits(cur.type,cur.rot,cur.x,cur.y+1)) cur.y++;
                else{lockPiece();afterLock();spawnNext();}
            }
        }
        doRender();SDL_Delay(16);
    }
    addScore(score);
    {std::ofstream f("scores.dat");for(int s:lb)f<<s<<"\n";}
    TTF_CloseFont(fSm);TTF_CloseFont(fMd);TTF_CloseFont(fLg);
    TTF_Quit();SDL_DestroyRenderer(ren);SDL_DestroyWindow(win);SDL_Quit();
    return 0;
}