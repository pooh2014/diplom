/****************************************************************************
  FILE..........: unit1.cpp
  AUTHOR........: Shuvaev Andrey
  DESCRIPTION...: Building a network schedule
  COPYRIGHT.....: GPL
  HISTORY.......: DATE       COMMENT
                  ---------- --------------------------------------
                  7-09-2010 Created - Shuvaev Andrey
/****************************************************************************/

#include <vcl.h>
#include <math.h>
#include <iostream>
#include <fstream.h>
#include <vector>
#include <clipbrd.hpp>
#define RadPoint 8 //Радиус точки
#define ColorNoEnable cl3DLight //Цвет заблокированных элементов

#pragma hdrstop

#include "Unit1.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "CSPIN"
#pragma resource "*.dfm"

using namespace std;

struct point
{
  int x;
  int y;
};

//Структура для одной работы
struct WorkData
{
  int num;
  AnsiString title;
  int StartEvent;
  int EndEvent;
  int t;
  int Trn;
  int Tro;
  int Tpn;
  int Tpo;
  int Rp;
  int Rc;
  bool operand;
  bool split;
  bool operator !=(WorkData);
};

//Структура для одного события
struct EventData
{
  int num;
  int x;
  int y;
  bool operand;
};

TForm1 *Form1;

//Режимы
bool BMouseDown=false; //зажата кнопка мыши
bool ActionSelect=true; //выбор объектов
bool ActionSelectArea=false; //выбор области
bool ActionAddEvent=false; //добавление события
bool ActionAddWork=false; //добавление работы
bool ActionEditWork=false; //редактирование работы

bool flag=false;
point PosMouse;  //позиция мыши
int StartEvnAddWork=0; //начальное событие при добавлении работы
int EvnEditWork=0; //редактируемое событие работы
int EditWork=-1; //редактирумая работа
int StepGrid=10; //шаг сетки
float scale=1;  //масштаб
int DefRadius=30; //радиус
int DefSizeFont=10; //размер шрифта

TFont* FontText;
Graphics::TBitmap *BitMap;
std::vector<WorkData> WorksTable;
std::vector<EventData> EventsTable;

//Прототипы
int FindEvn(const int n);
void OutputParametrs();

//---------------------------------------------------------------------------

class TPublicGrid: public  TStringGrid
{
    public:
        using TStringGrid::DeleteRow;
};
//---------------------------------------------------------------------------

__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------

template< typename T >
T min( T a, T b )
{
  return a < b ? a : b;
}
//---------------------------------------------------------------------------

template< typename T >
T max( T a, T b )
{
  return a < b ? b : a;
}
//---------------------------------------------------------------------------

//Эквивалентность работ определяется по их параметрам
//Используется при расчете параметров
bool WorkData::operator !=(WorkData WD)
{
  //if (num!=WD.num) return true;
  //if (StartEvent!=WD.StartEvent) return true;
  //if (EndEvent!=WD.EndEvent) return true;
  //if (t!=WD.t) return true;
  if (Trn!=WD.Trn) return true;
  if (Tro!=WD.Tro) return true;
  if (Tpn!=WD.Tpn) return true;
  if (Tpo!=WD.Tpo) return true;
  if (Rp!=WD.Rp) return true;
  if (Rc!=WD.Rc) return true;
  return false;  
}
//---------------------------------------------------------------------------

void Init()
{
  //Путь к текущей папке
  Form1->OpenDialog1->InitialDir=ExtractFilePath(Application->ExeName);
  Form1->SaveDialog1->InitialDir=ExtractFilePath(Application->ExeName);
  Form1->SaveDialog2->InitialDir=ExtractFilePath(Application->ExeName);
  //Установить рисунок для кнопки Сетка
  Form1->ImageList1->GetBitmap(0, Form1->COnOffGrid->Glyph);
  WorksTable.clear();
  EventsTable.clear();
  BitMap=new Graphics::TBitmap;
  //BitMap->Monochrome=true;  //Монохром
  BitMap->Transparent=true;
  BitMap->TransparentColor=clWhite;
  BitMap->Width=Form1->ScrollBox1->Width;
  BitMap->Height=Form1->ScrollBox1->Height;
  FontText=new TFont;
  FontText->Size=int(scale*DefSizeFont);
  BitMap->Canvas->Font=FontText;
  Form1->ScrollBox1->DoubleBuffered=true;
  Form1->ImageGraf->Width=BitMap->Width;
  Form1->ImageGraf->Height=BitMap->Height;
}

//---------------------------------------------------------------------------

//Сменить режим работы с объектами
void ChangeMode(int m)
{
  ActionSelect=false;
  ActionAddEvent=false;
  ActionAddWork=false;
  ActionEditWork=false;
  switch (m)
  {
    case 0: ActionSelect=true; break;
    case 1: ActionAddEvent=true; break;
    case 2: ActionAddWork=true; break;
    case 3: ActionEditWork=true; break;
  }
}

//---------------------------------------------------------------------------
/****************************************************************************
  START BLOCK DRAWING
/****************************************************************************/
//---------------------------------------------------------------------------

//Граница круга
//В качестве параметров передаются координаты центров двух кругов
//Возвращается точка на границе круга принадлежащая прямой, соединяющей эти круги
point PosBorderCircle(int x, int y, int x2, int y2)
{
  point p;
  int ScaleRadius;
  float BE;
  ScaleRadius=int(DefRadius*scale);
  if ((abs(x-x2)*abs(x-x2)+abs(y-y2)*abs(y-y2))==0)
    BE=0;
  else
    BE=abs(y-y2)*ScaleRadius/sqrt(abs(x-x2)*abs(x-x2)+abs(y-y2)*abs(y-y2));
  p.x=int(sqrt(ScaleRadius*ScaleRadius-BE*BE));
  if (x<x2)
    p.x=-p.x;
  p.x+=x2;
  if (y<y2)
    p.y=y2-int(BE);
  else
    p.y=y2+int(BE);
  return p;
}
//---------------------------------------------------------------------------

//Координаты текста
point PosText(int x, int y, int x2, int y2, AnsiString text)
{
  float s;
  point p;
  int w;
  int h;
  int z=1; //отображать справа
  w=int(BitMap->Canvas->TextWidth(text)/2);
  h=int(BitMap->Canvas->TextHeight(text)/2);
  //Вычисляем синус угла наклона прямой
  s=sin(abs(y-y2)/sqrt(abs(y-y2)*abs(y-y2)+abs(x-x2)*abs(x-x2)));
  //Если наклон вправо, то отображать слева
  if ((x>x2 & y<y2) || (x2>x & y2<y))
    z=-1;
  p.x=abs(x-x2)/2+min(x,x2)-w+z*s*30;
  p.y=abs(y-y2)/2+min(y,y2)-h*(1-s)-15;
  return p;
}
//---------------------------------------------------------------------------

//Нарисовать работу(стрелку)
void DrawWork(const int x1o, const int y1o, const int x2o, const int y2o,
  const AnsiString text, int Type, int fiq=1, bool split=false, int num=-1,
  bool EditStart=false, bool EditEnd=false)
{
  //Если работа разбита
  if(split)
  {
    //Нарисовать первую половину работы
    DrawWork(x1o,y1o,x1o+DefRadius*4,y1o,text,Type, fiq);
    BitMap->Canvas->TextOutA(x1o+int(DefRadius*1.2),y1o+10,
      "к событию "+IntToStr(num));
    //Нарисовать вторую половину работы
    DrawWork(x2o-DefRadius*4,y2o,x2o,y2o,text,Type, fiq);
    return;
  }
  TPoint points[3];
  long h,w,x3,y3,x4,y4,x5,y5,i,j;
  float nx,ny,Vx,Vy,V;
  //h и w - размеры рисуемой стрелки
  if (Type==0)
  {
    h=15;
    w=int(6-1+scale);
  }
  else
  {
    h=12;
    w=int(4-1+scale);
  }
  int x1=x1o;
  int y1=y1o;
  int x2=x2o;
  int y2=y2o;
  point p;
  //Если не редактируется начало работы
  if(!EditStart)
  {
    x1*=scale;
    y1*=scale;
  }
  //Если не редактируется конец работы
  if(!EditEnd)
  {
    x2*=scale;
    y2*=scale;
  }
  //Если не редактируется начало работы
  if(!EditStart)
  {
    //Определяем границу 1-го круга
    p=PosBorderCircle(x2, y2, x1, y1);
    x1=p.x;
    y1=p.y;
  }
  BitMap->Canvas->MoveTo(x1,y1);
  //Если не редактируется конец работы
  if(!EditEnd)
  {
    //Определяем границу 2-го круга
    p=PosBorderCircle(x1, y1, x2, y2);
    x2=p.x;
    y2=p.y;
  }
  Vx=x2-x1;
  Vy=y2-y1;
  V=sqrt(Vx*Vx+Vy*Vy);
  Vx=Vx/V;
  Vy=Vy/V;
  //Вектор n - перпендикулярен направлению движения
  nx=Vy;
  ny=-Vx;
  //Точка (x3,y3) лежит на прямой проходящей из 1 в 2 и находящейся
  //..на расстоянии h от точки 2
  x3=x2-int(h*Vx);
  y3=y2-int(h*Vy);
  //Точки (x4,y4) и (x5,y5) расположенны "по бокам" - зазубрины стрелки
  x4=x3+int(w*nx);
  y4=y3+int(w*ny);
  x5=x3-int(w*nx);
  y5=y3-int(w*ny);
  if(fiq==0)
    BitMap->Canvas->Pen->Style=psDot;
  switch (Type)
  {
    //Критический путь
    case 0:  BitMap->Canvas->Pen->Width=int(4*scale);
             BitMap->Canvas->LineTo(x3,y3);
             BitMap->Canvas->Pen->Width=1;
             break;
    //Не критический путь
    case 1:  BitMap->Canvas->LineTo(x2,y2);
             break;
    //Режим редактирования         
    case 2:  BitMap->Canvas->Brush->Color = clBlack;
             BitMap->Canvas->Ellipse(x1-4,y1-4,x1+4,y1+4);
             BitMap->Canvas->Ellipse(x2-4,y2-4,x2+4,y2+4);
             BitMap->Canvas->Brush->Color = clWhite;
             BitMap->Canvas->LineTo(x2,y2);
  }
  BitMap->Canvas->Pen->Style=psSolid;
  //Нарисовать стрелку
  points[0] = Point(x2,y2);
  points[1] = Point(x4,y4);
  points[2] = Point(x5,y5);
  BitMap->Canvas->Brush->Color = clBlack;
  BitMap->Canvas->Polygon(points,3);
  BitMap->Canvas->Brush->Color = clWhite;
  //Нарисовать текст
  p=PosText(x1, y1, x2, y2, text);
  BitMap->Canvas->TextOutA(p.x,p.y,text);
}
//---------------------------------------------------------------------------

//Нарисовать событие
void DrawEvent(const int x2, const int y2, const int num, const int EarlyDate,
  const int LaterDate, const int Reserve, const bool operand)
{
  int x=x2*scale;
  int y=y2*scale;
  int w;
  int h;
  int ScaleRad=int(DefRadius*scale);
  BitMap->Canvas->Ellipse(x-ScaleRad,y-ScaleRad,x+ScaleRad,y+ScaleRad);
  w=BitMap->Canvas->TextWidth(num);
  h=BitMap->Canvas->TextHeight(num);
  if(scale>0.5)
  {
    BitMap->Canvas->MoveTo(x-int(ScaleRad*2/3),y-int(ScaleRad*2/3));
    BitMap->Canvas->LineTo(x+int(ScaleRad*2/3),y+int(ScaleRad*2/3));
    BitMap->Canvas->MoveTo(x+int(ScaleRad*2/3),y-int(ScaleRad*2/3));
    BitMap->Canvas->LineTo(x-int(ScaleRad*2/3),y+int(ScaleRad*2/3));
    BitMap->Canvas->TextOutA(x-int(w/2),y+int(ScaleRad/2-h/2)+3,num);
    w=BitMap->Canvas->TextWidth(EarlyDate);
    h=BitMap->Canvas->TextHeight(EarlyDate);
    BitMap->Canvas->TextOutA(x-int(ScaleRad/2+w/2)-3,y-int(h/2),EarlyDate);
    w=BitMap->Canvas->TextWidth(LaterDate);
    h=BitMap->Canvas->TextHeight(LaterDate);
    BitMap->Canvas->TextOutA(x+int(ScaleRad/2-w/2)+3,y-int(h/2),LaterDate);
    w=BitMap->Canvas->TextWidth(Reserve);
    h=BitMap->Canvas->TextHeight(Reserve);
    BitMap->Canvas->TextOutA(x-int(w/2),y-int(ScaleRad/2+h/2)-3,Reserve);
  }
  else
  {
    BitMap->Canvas->TextOutA(x-int(w/2),y-int(h/2),num);
  }
  //Если событие выделено, то нарисовать 4 квадратика
  if(operand)
  {
    BitMap->Canvas->Brush->Color=clBlack;
    BitMap->Canvas->Rectangle(x-ScaleRad,y-ScaleRad,x-ScaleRad+3,y-ScaleRad+3);
    BitMap->Canvas->Rectangle(x+ScaleRad,y-ScaleRad,x+ScaleRad-3,y-ScaleRad+3);
    BitMap->Canvas->Rectangle(x-ScaleRad,y+ScaleRad,x-ScaleRad+3,y+ScaleRad-3);
    BitMap->Canvas->Rectangle(x+ScaleRad,y+ScaleRad,x+ScaleRad-3,y+ScaleRad-3);
    BitMap->Canvas->Brush->Color=clWhite;
  }
}
//---------------------------------------------------------------------------

//Рисование сетки
void DrawGrid()
{
  BitMap->Canvas->Pen->Color=clLtGray;
  for(int i=StepGrid*scale;i<BitMap->Height/scale;i+=StepGrid)
  {
    BitMap->Canvas->MoveTo(0,int(i*scale));
    BitMap->Canvas->LineTo(BitMap->Width,int(i*scale));
  }
  for(int i=StepGrid;i<BitMap->Width/scale;i+=StepGrid)
  {
    BitMap->Canvas->MoveTo(int(i*scale),0);
    BitMap->Canvas->LineTo(int(i*scale),BitMap->Height);
  }
  BitMap->Canvas->Pen->Color=clBlack;
}

//---------------------------------------------------------------------------

//Нарисовать ВСЕ
void DrawAll()
{
  int x;
  int y;
  int x2;
  int y2;
  int FirstEvent;
  int LastEvent;
  int MinRezerv;
  bool DrawStart;
  bool DrawEnd;
  PatBlt(BitMap->Canvas->Handle,0,0,BitMap->Width,BitMap->Height,WHITENESS);
  BitMap->Canvas->Font->Size=int(scale*DefSizeFont);
  //Если нережим выделения области, то расширить область рисования
  if(!ActionSelectArea)
  {
    int mx=Form1->ScrollBox1->Width;
    int my=Form1->ScrollBox1->Height;
    for(int i=0; i<EventsTable.size(); i++)
    {
      mx=max((EventsTable[i].x+DefRadius*6)*scale,mx);
      my=max((EventsTable[i].y+DefRadius*6)*scale,my);
    }
    BitMap->Width=mx;
    BitMap->Height=my;
    Form1->ImageGraf->Width=BitMap->Width;
    Form1->ImageGraf->Height=BitMap->Height;
  }

  if(StepGrid>1)
    DrawGrid();
  //Рисуем события
  for(int i=0; i<EventsTable.size();i++)
    DrawEvent(EventsTable[i].x,EventsTable[i].y,EventsTable[i].num,0,0,0,
    EventsTable[i].operand);

  for(int i=0; i<WorksTable.size();i++)
  {
    DrawStart=false;  //Рисование начального события работы запрещено
    DrawEnd=true; //Рисование конечного события работы запрещено
    MinRezerv=i; //Резерв времени
    for(int j=0; j<WorksTable.size();j++)
    {
      //Если есть последующие работы, то конечное событие не выводится
      if(WorksTable[i].EndEvent==WorksTable[j].StartEvent)
        DrawEnd=false;
      //Найти минимальный резерв из смежных работ
      if(WorksTable[i].StartEvent==WorksTable[j].StartEvent & i!=j)
      {
        if(WorksTable[MinRezerv].Rp>WorksTable[j].Rp)
          MinRezerv=j;
      }
    }
    //Если данная работа обладает минимальным резервом,
    //..то рисовать её начальное событие
    if(MinRezerv==i)
      DrawStart=true;
    FirstEvent=WorksTable[i].StartEvent;
    x=EventsTable[FirstEvent].x;
    y=EventsTable[FirstEvent].y;
    //Нарисовать начальное событие
    if(DrawStart)
    {
      DrawEvent(EventsTable[FirstEvent].x,EventsTable[FirstEvent].y,
        EventsTable[FirstEvent].num,
        WorksTable[i].Trn,
        WorksTable[i].Tpn,
        WorksTable[i].Rp,
        EventsTable[FirstEvent].operand);
    }
    LastEvent=WorksTable[i].EndEvent;
    x2=EventsTable[LastEvent].x;
    y2=EventsTable[LastEvent].y;
    //Нарисовать конечное событие
    if(DrawEnd)
    {
      DrawEvent(EventsTable[LastEvent].x,EventsTable[LastEvent].y,
        EventsTable[LastEvent].num,
        WorksTable[i].Tro,
        WorksTable[i].Tpo,
        WorksTable[i].Rp,
        EventsTable[LastEvent].operand);
    }
    AnsiString s="";
    if(Form1->CheckBox1->Checked)
    {
      if(Form1->Edit1->Text.Length()>1)
        s+=Form1->Edit1->Text[1];
      s+=IntToStr(WorksTable[i].Tpn-WorksTable[i].Trn);
      if(Form1->Edit1->Text.Length()>0)
        s+=Form1->Edit1->Text[Form1->Edit1->Text.Length()];
    }
    if(Form1->CheckBox2->Checked)
      s+=IntToStr(WorksTable[i].t);
    if(Form1->CheckBox3->Checked)
    {
      if(Form1->Edit1->Text.Length()>0)
        s+=Form1->Edit1->Text[1];
      s+=IntToStr(WorksTable[i].Rp-WorksTable[i].Tpo+WorksTable[i].Tro);
      if(Form1->Edit1->Text.Length()>1)
        s+=Form1->Edit1->Text[Form1->Edit1->Text.Length()];
    }
    //Если работа выделена
    if(WorksTable[i].operand)
    {
      //Если не режим редактирования
      if(!ActionEditWork)
        DrawWork(x, y, x2, y2, s, 2,WorksTable[i].t,WorksTable[i].split,
        EventsTable[WorksTable[i].EndEvent].num);
    }
    else
    {
      //Если резерв=0 то это критический путь
      if (WorksTable[i].Rp==0)
        DrawWork(x, y, x2, y2, s, 0,WorksTable[i].t,WorksTable[i].split,
        EventsTable[WorksTable[i].EndEvent].num);
      else
        DrawWork(x, y, x2, y2, s, 1,WorksTable[i].t,WorksTable[i].split,
        EventsTable[WorksTable[i].EndEvent].num);
    }
  }
  BitMap->Transparent=false;
  Form1->ImageGraf->Canvas->Draw(0,0,BitMap);
  BitMap->Transparent=true;
}
//---------------------------------------------------------------------------
/****************************************************************************
  END BLOCK DRAWING
/****************************************************************************/

/****************************************************************************
  START CREATION OF DATA STRUCTURE
/****************************************************************************/
//---------------------------------------------------------------------------

//Собрать работу
WorkData StringToStruct(
        const int num,
        const AnsiString title="",
        const int t=1,
        const int StartEvent=0,
        const int EndEvent=0,
        const int Trn=0,
        const int Tro=0,
        const int Tpn=0,
        const int Tpo=0,
        const int Rp=0,
        const int Rc=0,
        const bool split=false
        )
{
  WorkData WD;
  WD.num=num;
  WD.title=title;
  WD.t=t;
  WD.StartEvent=StartEvent;
  WD.EndEvent=EndEvent;
  WD.Trn=Trn;
  WD.Tro=Tro;
  WD.Tpn=Tpn;
  WD.Tpo=Tpo;
  WD.Rp=Rp;
  WD.Rc=Rc;
  WD.operand=false;
  WD.split=split;
  return WD;
}
//---------------------------------------------------------------------------

//Поиск события по номеру
int FindEvn(const int n)
{
  int i;
  for(i=0; i<EventsTable.size(); i++)
  {
    if (EventsTable[i].num==n)
      break;
  }
  if (i>=EventsTable.size())
    i=-1;
  return i;
}
//---------------------------------------------------------------------------

//Добавить событие
void AddEvent(const int x=-1, const int y=-1)
{
  EventData ED;
  bool b=false;
  int j=-1;
  //Поиск свободного номера
  while(!b)
  {
    j++;
    b=true;
    for(int i=0; i<EventsTable.size();i++)
    {
      if(EventsTable[i].num==j)
      {
        b=false;
        break;
      }
      EventsTable[i].operand=false;
    }
  }
  for(int i=0; i<WorksTable.size();i++)
    WorksTable[i].operand=false;
  ED.num=j;
  ED.x=x;
  ED.y=y;
  ED.operand=true;
  EventsTable.push_back(ED);
}

//---------------------------------------------------------------------------

//Удалить работу
void DelWork(const int num)
{
  WorksTable.erase(WorksTable.begin()+num,WorksTable.begin()+num+1);
}
//---------------------------------------------------------------------------

//Удалить событие
void DelEvn(const int evnt)
{
  EventsTable.erase(EventsTable.begin()+evnt, EventsTable.begin()+evnt+1);
  for (int i=0; i<WorksTable.size(); i++)
  {
    //Удалить работу используюшую данное событие
    if (WorksTable[i].StartEvent==evnt || WorksTable[i].EndEvent==evnt)
    {
      DelWork(i);
      i--;
    }
    //Сдвиг номеров событий для работ    
    else
    {
      if (WorksTable[i].StartEvent>evnt)
        WorksTable[i].StartEvent-=1;
      if (WorksTable[i].EndEvent>evnt)
        WorksTable[i].EndEvent-=1;
    }
  }
}
//---------------------------------------------------------------------------

//Расчет ранних сроков работы
void CalcEarlyPrmtWork(const int n)
{
  int m=0;
  bool b=false;
  for(int i=0;i<WorksTable.size();i++)
  {
     if(WorksTable[n].StartEvent==WorksTable[i].EndEvent)
     {
       b=true;
       break;
     }
  }
  //Trn
  //Если это первая работа
  if (!b)
    WorksTable[n].Trn=0;
  else
  {
    for(int i=0; i<WorksTable.size(); i++)
    {
      if (WorksTable[i].EndEvent==WorksTable[n].StartEvent)
        m=max(WorksTable[i].Trn+WorksTable[i].t,m);
    }
    WorksTable[n].Trn=m;
  }
  //Tro
  m=WorksTable[n].Trn+WorksTable[n].t;
  for(int i=0; i<WorksTable.size(); i++)
  {
    if (WorksTable[i].EndEvent==WorksTable[n].EndEvent)
      m=max(WorksTable[i].Trn+WorksTable[i].t,m);
  }
  WorksTable[n].Tro=m;
}
//---------------------------------------------------------------------------

//Расчет поздних сроков работы
void CalcLaterPrmtWork(const int n)
{
  int m;
  bool b=false;
  //Tpn
  m=5000;
  for(int i=0; i<WorksTable.size(); i++)
  {
    if (WorksTable[i].StartEvent==WorksTable[n].EndEvent)
      m=min(WorksTable[i].Tpn-WorksTable[n].t,m);
  }
  /*for(int i=0; i<WorksTable.size(); i++)
  {
    if (WorksTable[i].StartEvent==WorksTable[n].StartEvent & i!=n & WorksTable[i].Tpo>0)
    {
      m=min(WorksTable[i].Tpo-WorksTable[i].t,m);
    }
  }*/
  if (m==5000)
  {
    //WorksTable[n].Tpn=WorksTable[n].Trn;
    WorksTable[n].Tpn=max(WorksTable[n].Trn,WorksTable[n].Tpo-WorksTable[n].t);
  }
  else
    WorksTable[n].Tpn=m;
  //Tpo
  m=WorksTable[n].Tro;
  for(int i=0; i<WorksTable.size(); i++)
  {
    if (WorksTable[i].StartEvent==WorksTable[n].EndEvent)
    {
      if (!b)
        m=WorksTable[i].Tpn;
      else
        m=min(WorksTable[i].Tpn,m);
      b=true;
    }
  }
  WorksTable[n].Tpo=m;
}
//---------------------------------------------------------------------------

//Расчет параметров работы
void CalcRPrmtWorks(const int n)
{
  WorksTable[n].Rp=WorksTable[n].Tpo-WorksTable[n].Trn-WorksTable[n].t;
  WorksTable[n].Rc=WorksTable[n].Tro-WorksTable[n].Trn-WorksTable[n].t;
}
//---------------------------------------------------------------------------

//Расчет параметров всех работ
void CalcPrmtAllWorks()
{
  bool b=true;
  std::vector<WorkData> WorksTable2;
  //Повторять пока есть изменения после расчетов
  while (b)
  {
    WorksTable2.clear();
    b=false;
    //Создать копию работ
    for (int i=0;i<WorksTable.size();i++)
      WorksTable2.push_back(WorksTable[i]);
    //Расчет ранних сроков работы
    for (int i=0; i<WorksTable.size();i++)
      CalcEarlyPrmtWork(i);
    //Расчет поздних сроков работы
    for (int i=WorksTable.size()-1; i>-1;i--)
      CalcLaterPrmtWork(i);
    //Сравнить работы после расчетов
    for (int i=0;i<WorksTable.size();i++)
    {
      if (WorksTable[i]!=WorksTable2[i])
      {
        b=true;
        break;
      }
    }
  }
  for (int i=WorksTable.size()-1; i>-1;i--)
    CalcRPrmtWorks(i);
  WorksTable2.clear();
}

//---------------------------------------------------------------------------

//Вывести параметры работы на правой панели
void TForm1::OutPrmtWork(const int num)
{
  flag=true;
  CEditTitleWork->Text=WorksTable[num].title;
  CEditTWork->Value=WorksTable[num].t;
  CEditStartWork->Value=FindEvn(WorksTable[num].StartEvent);
  CEditEndWork->Value=FindEvn(WorksTable[num].EndEvent);
  for(int i=0;i<EventsTable.size();i++)
    EventsTable[i].operand=false;
  Panel3->Enabled=true;
  CEditTitleWork->Color=clWindow;
  CEditTWork->Color=clWindow;
  CEditStartWork->Color=clWindow;
  CEditEndWork->Color=clWindow;
  Panel1->Enabled=false;
  CEditNumEvn->Color=ColorNoEnable;
  flag=false;
}
//---------------------------------------------------------------------------

//Вывести параметры события на правой панели
void TForm1::OutPrmtEvent(const int num)
{
  flag=true;
  CEditNumEvn->Text=EventsTable[num].num;
  Panel1->Enabled=true;
  CEditNumEvn->Color=clWindow;
  Panel3->Enabled=false;
  CEditTitleWork->Color=ColorNoEnable;
  CEditTWork->Color=ColorNoEnable;
  CEditStartWork->Color=ColorNoEnable;
  CEditEndWork->Color=ColorNoEnable;
  flag=false;
}
//---------------------------------------------------------------------------

//Загрузка
void Deserialization(AnsiString FileName)
{
  ifstream in_file(FileName.c_str());
  in_file.seekg(0,std::ios::end);
  long size = in_file.tellg();
  char *buf = new char[size+1];
  char *buf2;
  char *open;
  char *open2;
  char *close;
  in_file.seekg(0);
  in_file.read(buf,size);
  buf[size]=0;
  //Загрузка работ
  buf2=strstr(buf,"<work>");
  while(buf2!=NULL)
  {
    int num=0;
    AnsiString title="";
    int StartEvent=0;
    int EndEvent=0;
    int t=1;
    int Trn=0;
    int Tro=0;
    int Tpn=0;
    int Tpo=0;
    int Rp=0;
    int Rc=0;
    bool Split=false;
    char *name;
    do
    {
      open=strstr(buf2+1,"<")+1;
      open2=strstr(open+1,"<");
      close=strstr(open,">");
      int len=close-open;
      name=new char[len+1];
      name[len]=0;
      strncpy (name,open,len);
      len=open2-close-1;
      char *val=new char[len+1];
      val[len]=0;
      strncpy (val,close+1,len);
      if(AnsiString(name)=="num") num=StrToInt(val);
      if(AnsiString(name)=="title") title=val;
      if(AnsiString(name)=="StartEvent") StartEvent=StrToInt(val);
      if(AnsiString(name)=="EndEvent") EndEvent=StrToInt(val);
      if(AnsiString(name)=="t") t=StrToInt(val);
      if(AnsiString(name)=="Trn") Trn=StrToInt(val);
      if(AnsiString(name)=="Tro") Tro=StrToInt(val);
      if(AnsiString(name)=="Tpn") Tpn=StrToInt(val);
      if(AnsiString(name)=="Tpo") Tpo=StrToInt(val);
      if(AnsiString(name)=="Rp") Rp=StrToInt(val);
      if(AnsiString(name)=="Rc") Rc=StrToInt(val);
      if(AnsiString(name)=="Split") Split=StrToBool(val);
      buf2=open2+1;
    }while(AnsiString(name)!="Split");
    WorksTable.push_back(StringToStruct(
      num,
      title,
      t,
      StartEvent,
      EndEvent,
      Trn,
      Tro,
      Tpn,
      Tpo,
      Rp,
      Rc,
      Split
    ));
    buf2=strstr(buf2,"<work>");
  }
  //Загрузка событий
  buf2=strstr(buf,"<event>");
  while(buf2!=NULL)
  {
    EventData ED;
    ED.num=0;
    ED.x=0;
    ED.y=0;
    ED.operand=0;
    char *name;
    do
    {
      open=strstr(buf2+1,"<")+1;
      open2=strstr(open+1,"<");
      close=strstr(open,">");
      int len=close-open;
      name=new char[len+1];
      name[len]=0;
      strncpy (name,open,len);
      len=open2-close-1;
      char *val=new char[len+1];
      val[len]=0;
      strncpy (val,close+1,len);
      if(AnsiString(name)=="num") ED.num=StrToInt(val);
      if(AnsiString(name)=="x") ED.x=StrToInt(val);
      if(AnsiString(name)=="y") ED.y=StrToInt(val);
      buf2=open2+1;
    }while(AnsiString(name)!="y");
    EventsTable.push_back(ED);
    buf2=strstr(buf2+1,"<event>");
  }
  delete[] buf;
  delete[] buf2;
  in_file.clear();
  in_file.close();
  DrawAll();
  OutputParametrs();
}

//---------------------------------------------------------------------------

//Сохранение
void Serialization(AnsiString FileName)
{
  ofstream out_file(FileName.c_str());
  out_file<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"<<'\n';
  out_file<<"<works_table>"<<'\n';
  for(int i=0;i<WorksTable.size();i++)
  {
    out_file<<"  <work>"<<'\n';
    out_file<<"    <num>"<<IntToStr(WorksTable[i].num).c_str()<<"</num>\n";
    out_file<<"    <title>"<<WorksTable[i].title.c_str()<<"</title>\n";
    out_file<<"    <StartEvent>"<<IntToStr(WorksTable[i].StartEvent).c_str()<<"</StartEvent>\n";
    out_file<<"    <EndEvent>"<<IntToStr(WorksTable[i].EndEvent).c_str()<<"</EndEvent>\n";
    out_file<<"    <t>"<<IntToStr(WorksTable[i].t).c_str()<<"</t>\n";
    out_file<<"    <Trn>"<<IntToStr(WorksTable[i].Trn).c_str()<<"</Trn>\n";
    out_file<<"    <Tro>"<<IntToStr(WorksTable[i].Tro).c_str()<<"</Tro>\n";
    out_file<<"    <Tpn>"<<IntToStr(WorksTable[i].Tpn).c_str()<<"</Tpn>\n";
    out_file<<"    <Tpo>"<<IntToStr(WorksTable[i].Tpo).c_str()<<"</Tpo>\n";
    out_file<<"    <Rp>"<<IntToStr(WorksTable[i].Rp).c_str()<<"</Rp>\n";
    out_file<<"    <Rc>"<<IntToStr(WorksTable[i].Rc).c_str()<<"</Rc>\n";
    out_file<<"    <Split>"<<BoolToStr(WorksTable[i].split).c_str()<<"</Split>\n";
    out_file<<"  </work>\n";
  }
  out_file<<"</works_table>\n";
  out_file<<"<events_table>\n";
  for(int i=0;i<EventsTable.size();i++)
  {
    out_file<<"  <event>\n";
    out_file<<"    <num>"<<EventsTable[i].num<<"</num>\n";
    out_file<<"    <x>"<<IntToStr(EventsTable[i].x).c_str()<<"</x>\n";
    out_file<<"    <y>"<<IntToStr(EventsTable[i].y).c_str()<<"</y>\n";
    out_file<<"  </event>\n";
  }
  out_file<<"</events_table>";
  out_file.clear();
  out_file.close();
  ShowMessage("Сохранено!");
}

//---------------------------------------------------------------------------
/****************************************************************************
  END CREATION OF DATA STRUCTURE
/****************************************************************************/

/****************************************************************************
  START INTERFACE
/****************************************************************************/
//---------------------------------------------------------------------------

void __fastcall TForm1::FormCreate(TObject *Sender)
{
  Init();
  GridParametrs->Cells[0][0]="Код";
  GridParametrs->Cells[1][0]="Название";
  GridParametrs->Cells[2][0]="t";
  GridParametrs->Cells[3][0]="Трн";
  GridParametrs->Cells[4][0]="Тро";
  GridParametrs->Cells[5][0]="Тпн";
  GridParametrs->Cells[6][0]="Тпо";
  GridParametrs->Cells[7][0]="Rп";
  GridParametrs->Cells[8][0]="Rс";
  DrawAll();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
  delete BitMap;
}
//---------------------------------------------------------------------------

//Вывод параметров работ
void OutputParametrs()
{
  Form1->GridParametrs->RowCount=WorksTable.size()+2;
  Form1->GridParametrs->Rows[Form1->GridParametrs->RowCount-1]->Clear();
  for (int i=0; i<WorksTable.size();i++)
  {
    Form1->GridParametrs->Cells[0][i+1]=
      IntToStr(FindEvn(WorksTable[i].StartEvent))+"-"+
      IntToStr(FindEvn(WorksTable[i].EndEvent));
    Form1->GridParametrs->Cells[1][i+1]=WorksTable[i].title;
    Form1->GridParametrs->Cells[2][i+1]=WorksTable[i].t;
    Form1->GridParametrs->Cells[3][i+1]=WorksTable[i].Trn;
    Form1->GridParametrs->Cells[4][i+1]=WorksTable[i].Tro;
    Form1->GridParametrs->Cells[5][i+1]=WorksTable[i].Tpn;
    Form1->GridParametrs->Cells[6][i+1]=WorksTable[i].Tpo;
    Form1->GridParametrs->Cells[7][i+1]=WorksTable[i].Rp;
    Form1->GridParametrs->Cells[8][i+1]=WorksTable[i].Rc;
  }
}
//---------------------------------------------------------------------------

//Расчет параметров работ
void CalcPrmt()
{
  CalcPrmtAllWorks();
  OutputParametrs();
  DrawAll();
}
//---------------------------------------------------------------------------

//Масштаб

//Шаг сетки
void __fastcall TForm1::CSpinEdit1Change(TObject *Sender)
{
  if(CSpinEdit1->Text!="")
    StepGrid=CSpinEdit1->Value;
  DrawAll();  
}
//---------------------------------------------------------------------------

//Вкл/откл сетку
void __fastcall TForm1::COnOffGridClick(TObject *Sender)
{
  if(COnOffGrid->Hint=="Нет сетки")
  {
    StepGrid=CSpinEdit1->Value;
    COnOffGrid->Hint="Сетка";
    CSpinEdit1->Enabled=true;
    CSpinEdit1->Color=clWindow;
    COnOffGrid->Glyph=NULL;
    ImageList1->GetBitmap(0, COnOffGrid->Glyph);
  }
  else
  {
    StepGrid=1;
    COnOffGrid->Hint="Нет сетки";
    CSpinEdit1->Enabled=false;
    CSpinEdit1->Color=ColorNoEnable;
    COnOffGrid->Glyph=NULL;
    ImageList1->GetBitmap(1, COnOffGrid->Glyph);
  }
  DrawAll();
}
//---------------------------------------------------------------------------

//Разбить работу
void __fastcall TForm1::SpeedButton1Click(TObject *Sender)
{
  for(int i=0;i<WorksTable.size();i++)
  {
    if((EventsTable[WorksTable[i].StartEvent].operand &
      EventsTable[WorksTable[i].EndEvent].operand) || WorksTable[i].operand)
      WorksTable[i].split=!WorksTable[i].split;
  }
  DrawAll();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ImageGrafMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
  //Вход в режим редактирования стрелки - нажатие на концы
  if(ActionSelect)
  {
      int x1,y1,x2,y2;
      point p1,p2;
      for(int i=0;i<WorksTable.size();i++)
      {
        if(WorksTable[i].split)
          continue;
        x1=EventsTable[WorksTable[i].StartEvent].x;
        y1=EventsTable[WorksTable[i].StartEvent].y;
        x2=EventsTable[WorksTable[i].EndEvent].x;
        y2=EventsTable[WorksTable[i].EndEvent].y;
        p1=PosBorderCircle(x2*scale,y2*scale,x1*scale,y1*scale);
        p2=PosBorderCircle(x1*scale,y1*scale,x2*scale,y2*scale);
        x1=p1.x;
        y1=p1.y;
        x2=p2.x;
        y2=p2.y;
        //Если нажали на начало
        if(X>x1-RadPoint & X<x1+RadPoint &
          Y>y1-RadPoint & Y<y1+RadPoint)
        {
          ChangeMode(3);
          EditWork=i;
          WorksTable[i].operand=true;
          EvnEditWork=WorksTable[i].StartEvent;
        }
        //Если нажали на конец
        if(X>x2-RadPoint & X<x2+RadPoint &
          Y>y2-RadPoint & Y<y2+RadPoint)
        {
          ChangeMode(3);
          EditWork=i;
          WorksTable[i].operand=true;
          EvnEditWork=WorksTable[i].EndEvent;
        }
        if(!ActionEditWork)
          WorksTable[i].operand=false;
      }
  }
  //Выделение событий
  if(ActionSelect || ActionAddWork)
  {
    int x0;
    int y0;
    for(int i=0; i<EventsTable.size();i++)
    {
      x0=int(EventsTable[i].x*scale);
      y0=int(EventsTable[i].y*scale);
      //Если нажали на событие
      if(sqrt(abs(X-x0)*abs(X-x0)+abs(Y-y0)*abs(Y-y0))<DefRadius*scale)
      {
        //Снять выделение со всех работ
        for(int i=0;i<WorksTable.size();i++)
          WorksTable[i].operand=false;
        BMouseDown=true;
        //Если не зажали CTRL и данное событие не выделено
        //то сбросить выделение со всех событий
        if(!EventsTable[i].operand & (GetKeyState(VK_CONTROL) & 256)==0)
        {
          for(int j=0; j<EventsTable.size();j++)
            EventsTable[j].operand=false;
        }
        EventsTable[i].operand=true;
        StartEvnAddWork=i;
        break;
      }
    }
  } //if(ActionSelect || ActionAddWork)
  if(ActionSelect)
  {
    //Обработка нажатия на стрелку
    bool SelectWork=false;
    if(!BMouseDown)
    {
      int x1,y1,x2,y2,k;
      point p1,p2;
      for(int i=0;i<WorksTable.size();i++)
      {
        x1=EventsTable[WorksTable[i].StartEvent].x;
        y1=EventsTable[WorksTable[i].StartEvent].y;
        x2=EventsTable[WorksTable[i].EndEvent].x;
        y2=EventsTable[WorksTable[i].EndEvent].y;
        //Если работа не разбита
        if(!WorksTable[i].split)
        {
          p1=PosBorderCircle(x1,y1,x2,y2);
          p2=PosBorderCircle(x2,y2,x1,y1);
          x1=int(p1.x*scale);
          y1=int(p1.y*scale);
          x2=int(p2.x*scale);
          y2=int(p2.y*scale);
          if(X>=min(x1,x2)-2 & X<=max(x1,x2)+2 & Y>=min(y1,y2)-2 & Y<=max(y1,y2)+2)
          {
            //Вычислить принадлежность точки к прямой
            k=(y1-y2)*X+(x2-x1)*Y+x1*y2-x2*y1;
            if (k<600 & k>-600)
            {
              WorksTable[i].operand=true;
              SelectWork=true;
            }
          }
        }
        //Если работа разбита
        else
        {
          x1=int(x1*scale);
          y1=int(y1*scale);
          x2=int(x2*scale);
          y2=int(y2*scale);
          if((X>=x1-2 & X<=x1+DefRadius*3+2 & Y>=y1-2 & Y<=y1+2) ||
            (X>=x2-DefRadius*3-2 & X<=x2+2 & Y>=y2-2 & Y<=y2+2))
          {
              WorksTable[i].operand=true;
              SelectWork=true;
          }
        }
      }//for(int i=0;i<WorksTable.size();i++)
    } //if(!BMouseDown)

    //Если нажали на свободном поле
    if(!BMouseDown & !SelectWork)
    {
      for(int i=0; i<EventsTable.size();i++)
        EventsTable[i].operand=false;
      for(int i=0; i<WorksTable.size();i++)
        WorksTable[i].operand=false;
      ActionSelectArea=true; //Режим выделения области
    }
  }
  PosMouse.x=X;
  PosMouse.y=Y;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ImageGrafMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
  //Режим перемещения
  if(ActionSelect & BMouseDown)
  {
    float StepGridScale=StepGrid*scale;
    int x2=int(X/StepGridScale)*StepGrid-int(PosMouse.x/StepGridScale)*StepGrid;
    int y2=int(Y/StepGridScale)*StepGrid-int(PosMouse.y/StepGridScale)*StepGrid;
    if (x2!=0 || y2!=0)
    {
      for(int i=0; i<EventsTable.size();i++)
      {
        if(EventsTable[i].operand)
        {
          EventsTable[i].x=int(EventsTable[i].x/StepGrid)*StepGrid+x2;
          EventsTable[i].y=int(EventsTable[i].y/StepGrid)*StepGrid+y2;
        }
      }
      DrawAll();
    }
    PosMouse.x=X;
    PosMouse.y=Y;
  }
  //Режим выделения области
  if(ActionSelectArea)
  {
    int x1=int((min(X,PosMouse.x)-DefRadius)/scale);
    int x2=int((max(X,PosMouse.x)+DefRadius)/scale);
    int y1=int((min(Y,PosMouse.y)-DefRadius)/scale);
    int y2=int((max(Y,PosMouse.y)+DefRadius)/scale);
    DrawAll();
    BitMap->Canvas->Rectangle(X,Y,PosMouse.x,PosMouse.y);
    ImageGraf->Canvas->Draw(0,0,BitMap);
    for(int i=0; i<EventsTable.size();i++)
    {
      if(EventsTable[i].x>x1 & EventsTable[i].x<x2 & EventsTable[i].y>y1 &
        EventsTable[i].y<y2)
        EventsTable[i].operand=true;
      else
        EventsTable[i].operand=false;
    }
  }
  //Режим добавления работы
  if(ActionAddWork & BMouseDown)
  {
    DrawAll();
    int x0;
    int y0;
    int e=-1;
    for(int i=0; i<EventsTable.size();i++)
    {
      x0=int(EventsTable[i].x*scale);
      y0=int(EventsTable[i].y*scale);
      if(sqrt(abs(X-x0)*abs(X-x0)+abs(Y-y0)*abs(Y-y0))<DefRadius*scale)
      {
        e=i;
        break;
      }
    }
    //Если навели конец стрелки на событие, то примагнитится к нему 
    if(e>-1)
      DrawWork(EventsTable[StartEvnAddWork].x,EventsTable[StartEvnAddWork].y,
        EventsTable[e].x,EventsTable[e].y,"",2,1,false,-1,false,false);
    else
      DrawWork(EventsTable[StartEvnAddWork].x,EventsTable[StartEvnAddWork].y,
        X,Y,"",2,1,false,-1,false,true);
    Form1->ImageGraf->Canvas->Draw(0,0,BitMap);
  }
  //Режим редактирования стрелки
  if(ActionEditWork)
  {
    DrawAll();
    int x0;
    int y0;
    int e=-1;
    for(int i=0; i<EventsTable.size();i++)
    {
      x0=int(EventsTable[i].x*scale);
      y0=int(EventsTable[i].y*scale);
      if(sqrt(abs(X-x0)*abs(X-x0)+abs(Y-y0)*abs(Y-y0))<DefRadius*scale)
      {
        e=i;
        break;
      }
    }
    //Если навели стрелку на событие, то примагнитится к нему
    if(e>-1)
    {
      //Редактируется начало
      if(EvnEditWork==WorksTable[EditWork].StartEvent)
        DrawWork(EventsTable[e].x,EventsTable[e].y,
          EventsTable[WorksTable[EditWork].EndEvent].x,
          EventsTable[WorksTable[EditWork].EndEvent].y,"",2,1,false,-1,false,false);
      //Редактируется конец
      if(EvnEditWork==WorksTable[EditWork].EndEvent)
        DrawWork(EventsTable[WorksTable[EditWork].StartEvent].x,
          EventsTable[WorksTable[EditWork].StartEvent].y,
          EventsTable[e].x,EventsTable[e].y,"",2,false,-1,1,false,false);
    }
    else
    {
      //Редактируется начало
      if(EvnEditWork==WorksTable[EditWork].StartEvent)
        DrawWork(X,Y,EventsTable[WorksTable[EditWork].EndEvent].x,
          EventsTable[WorksTable[EditWork].EndEvent].y,"",2,1,false,-1,true,false);
      //Редактируется конец
      if(EvnEditWork==WorksTable[EditWork].EndEvent)
        DrawWork(EventsTable[WorksTable[EditWork].StartEvent].x,
          EventsTable[WorksTable[EditWork].StartEvent].y,X,Y,"",2,1,false,-1,false,true);
    }
    Form1->ImageGraf->Canvas->Draw(0,0,BitMap);
  }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ImageGrafMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
  BMouseDown=false;
  ActionSelectArea=false;
  //Режим добавления события  
  if(ActionAddEvent)
    AddEvent(int(X/(StepGrid*scale))*StepGrid,int(Y/(StepGrid*scale))*StepGrid);
  //Режим добавления работы
  if(ActionAddWork)
  {
    int x0;
    int y0;
    for(int i=0; i<EventsTable.size();i++)
    {
      x0=int(EventsTable[i].x*scale);
      y0=int(EventsTable[i].y*scale);
      if(sqrt(abs(X-x0)*abs(X-x0)+abs(Y-y0)*abs(Y-y0))<DefRadius*scale)
      {
        bool Exist=false;
        for(int j=0;j<WorksTable.size();j++)
        {
          if(WorksTable[j].StartEvent==StartEvnAddWork || WorksTable[j].StartEvent==i)
            if(WorksTable[j].EndEvent==StartEvnAddWork || WorksTable[j].EndEvent==i)
            {
              Exist=true;
              break;
            }
        }
        //Если не Начало не равно Концу и если нет параллельной работы
        if(StartEvnAddWork != i & !Exist)
        {
          WorksTable.push_back(StringToStruct(WorksTable.size(),"",1,StartEvnAddWork,i));
          WorksTable[WorksTable.size()-1].operand=true;
        }
        break;
      }
    }
  }
  //Режим редактирования работы
  if(ActionEditWork)
  {
    int x0;
    int y0;
    for(int i=0; i<EventsTable.size();i++)
    {
      x0=int(EventsTable[i].x*scale);
      y0=int(EventsTable[i].y*scale);
      if(sqrt(abs(X-x0)*abs(X-x0)+abs(Y-y0)*abs(Y-y0))<DefRadius*scale)
      {
        bool Exist=false;
        int EvnNoEditWork;
        if(WorksTable[EditWork].StartEvent!=EvnEditWork)
          EvnNoEditWork=WorksTable[EditWork].StartEvent;
        else
          EvnNoEditWork=WorksTable[EditWork].EndEvent;
        for(int j=0;j<WorksTable.size();j++)
        {
          if(WorksTable[j].StartEvent==EvnNoEditWork || WorksTable[j].StartEvent==i)
            if(WorksTable[j].EndEvent==EvnNoEditWork || WorksTable[j].EndEvent==i)
            {
              Exist=true;
              break;
            }
        }
        //Если не Начало не равно Концу и если нет параллельной работы
        if(EvnEditWork != i & !Exist)
        {
          if(EvnEditWork==WorksTable[EditWork].StartEvent)
            WorksTable[EditWork].StartEvent=i;
          if(EvnEditWork==WorksTable[EditWork].EndEvent)
            WorksTable[EditWork].EndEvent=i;
        }
        break;
      }
    }
    ChangeMode(0);
  }
  bool b=false;
  //Вывод параметров работы
  for(int i=0;i<WorksTable.size();i++)
  {
    if(WorksTable[i].operand)
    {
      OutPrmtWork(i);
      b=true;
      break;
    }
  }
  //Вывод параметров события
  for(int i=0;i<EventsTable.size();i++)
  {
    if(EventsTable[i].operand)
    {
      OutPrmtEvent(i);
      b=true;
      break;
    }
  }
  //Если нет выделенных объектов
  if(!b)
  {
    Panel1->Enabled=false;
    Panel3->Enabled=false;
    CEditTitleWork->Color=ColorNoEnable;
    CEditTWork->Color=ColorNoEnable;
    CEditStartWork->Color=ColorNoEnable;
    CEditEndWork->Color=ColorNoEnable;
    CEditNumEvn->Color=ColorNoEnable;
  }
  DrawAll();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ImageGrafPaint(TObject *Sender)
{
  Form1->ImageGraf->Canvas->Draw(0,0,BitMap);
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CSpinEdit1KeyPress(TObject *Sender, char &Key)
{
  if (!(Key<='9' & Key>='0' || Key==VK_BACK || Key==VK_DELETE))
    Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CModeSelectClick(TObject *Sender)
{
  ChangeMode(0); //Режим выделения
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CModeAddEvnClick(TObject *Sender)
{
  ChangeMode(1); //Режим добавления события
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CmodeAddWorkClick(TObject *Sender)
{
  ChangeMode(2); //Режим добавления работы
}
//---------------------------------------------------------------------------

//Удаление объектов
void __fastcall TForm1::CDelClick(TObject *Sender)
{
  for(int i=0; i<WorksTable.size(); i++)
  {
    if(WorksTable[i].operand)
    {
      DelWork(i);
      i--;
    }
  }
  for(int i=0; i<EventsTable.size();i++)
  {
    if(EventsTable[i].operand)
    {
      DelEvn(i);
      i--;
    }
  }
  DrawAll();
}
//---------------------------------------------------------------------------

//Редактирование названия работы
void __fastcall TForm1::CEditTitleWorkChange(TObject *Sender)
{
  for(int i=0;i<WorksTable.size();i++)
  {
    if(WorksTable[i].operand)
    {
      WorksTable[i].title=CEditTitleWork->Text;
      break;
    }
  }
}
//---------------------------------------------------------------------------

//Редактирование длительности работы
void __fastcall TForm1::CEditTWorkChange(TObject *Sender)
{
  if(!flag & CEditTWork->Text!="")
  for(int i=0;i<WorksTable.size();i++)
  {
    if(WorksTable[i].operand)
    {
      WorksTable[i].t=CEditTWork->Value;
      break;
    }
  }
  DrawAll();
}
//---------------------------------------------------------------------------

//Редактирование начального события
void __fastcall TForm1::CEditStartWorkChange(TObject *Sender)
{
  if(!flag & CEditStartWork->Text!="")
  for(int i=0;i<WorksTable.size();i++)
  {
    if(WorksTable[i].operand)
    {
      WorksTable[i].StartEvent=FindEvn(CEditStartWork->Value);
      break;
    }
  }
  DrawAll();  
}
//---------------------------------------------------------------------------

//Редактирование конечного события
void __fastcall TForm1::CEditEndWorkChange(TObject *Sender)
{
  if(!flag & CEditEndWork->Text!="")
  for(int i=0;i<WorksTable.size();i++)
  {
    if(WorksTable[i].operand)
    {
      WorksTable[i].EndEvent=FindEvn(CEditEndWork->Value);
      break;
    }
  }
  DrawAll();
}
//---------------------------------------------------------------------------

//Редактирование номера события
void __fastcall TForm1::CEditNumEvnChange(TObject *Sender)
{
  if(CEditNumEvn->Text!="")
  for(int i=0;i<EventsTable.size();i++)
  {
    if(EventsTable[i].operand)
    {
      EventsTable[i].num=CEditNumEvn->Value;
      break;
    }
  }
  DrawAll();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CLoadClick(TObject *Sender)
{
  if(OpenDialog1->Execute())
    Deserialization(OpenDialog1->FileName);        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CSaveClick(TObject *Sender)
{
  if(SaveDialog2->Execute())
    Serialization(SaveDialog2->FileName);        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CSaveImageClick(TObject *Sender)
{
  if(SaveDialog1->Execute())
  {
    /*Graphics::TBitmap *BitMap2;
    BitMap2=new Graphics::TBitmap;
    int mx=0;
    int my=0;
    //Найти границы графика
    for(int i=0;i<EventsTable.size();i++)
    {
      mx=max(mx,EventsTable[i].x);
      my=max(my,EventsTable[i].y);
    }
    BitMap2->Width=int((mx+50)*scale);
    BitMap2->Height=int((my+50)*scale);
    BitMap2->Canvas->Draw(0,0,BitMap);
    BitMap2->SaveToFile(SaveDialog1->FileName);
    delete BitMap2; */
    BitMap->SaveToFile(SaveDialog1->FileName);
    ShowMessage("Сохранено!");
  }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CCalcClick(TObject *Sender)
{
  CalcPrmt();        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CClearClick(TObject *Sender)
{
  Init();
  OutputParametrs();
  DrawAll;        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CEditTWorkKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
  if (!(Key<='9' & Key>='0' || Key==VK_BACK || Key==VK_DELETE))
    Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CEditStartWorkKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
  if (!(Key<='9' & Key>='0' || Key==VK_BACK || Key==VK_DELETE))
    Key=0;        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CEditEndWorkKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
  if (!(Key<='9' & Key>='0' || Key==VK_BACK || Key==VK_DELETE))
    Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CEditNumEvnKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
  if (!(Key<='9' & Key>='0' || Key==VK_BACK || Key==VK_DELETE))
    Key=0;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::ScrollBox1Resize(TObject *Sender)
{
  DrawAll();        
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CZoomInClick(TObject *Sender)
{
  if(scale<1)
  {
    scale+=0.1;
    DrawAll();
  }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::CZoomOutClick(TObject *Sender)
{
  if(scale>0.3)
  {
    scale-=0.1;
    DrawAll();
  }
}
//---------------------------------------------------------------------------

void __fastcall TForm1::SpeedButton2Click(TObject *Sender)
{
  AnsiString s="";
  for(int i=0; i<GridParametrs->RowCount;i++)
  {
    for(int j=0; j<GridParametrs->ColCount;j++)
    {
      s+=GridParametrs->Cells[j][i]+char(9);
    }
    s+=char(13);
    s+=char(10);
  }
  Clipboard()->AsText = s;
}
//---------------------------------------------------------------------------




