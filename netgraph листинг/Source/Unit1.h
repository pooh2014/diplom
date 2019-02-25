//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Grids.hpp>
#include <DBGrids.hpp>
#include <DBCtrls.hpp>
#include <ComCtrls.hpp>
#include <Graphics.hpp>
#include "CSPIN.h"
#include <Buttons.hpp>
#include <Menus.hpp>
#include <ToolWin.hpp>
#include <Dialogs.hpp>
#include <ImgList.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
        TPanel *Panel2;
        TScrollBox *ScrollBox1;
        TPageControl *PageControl1;
        TTabSheet *TabSheet1;
        TTabSheet *TabSheet2;
        TStringGrid *GridParametrs;
        TPaintBox *ImageGraf;
        TPopupMenu *PopupMenu2;
        TMenuItem *N2;
        TSaveDialog *SaveDialog1;
        TOpenDialog *OpenDialog1;
        TSaveDialog *SaveDialog2;
        TMenuItem *N12;
        TToolBar *ToolBar1;
        TCSpinEdit *CSpinEdit1;
        TSpeedButton *CModeSelect;
        TSpeedButton *CModeAddEvn;
        TSpeedButton *CmodeAddWork;
        TSpeedButton *CDel;
        TSpeedButton *COnOffGrid;
        TSpeedButton *CLoad;
        TSpeedButton *CSave;
        TSpeedButton *CSaveImage;
        TSpeedButton *CCalc;
        TSpeedButton *CClear;
        TToolBar *ToolBar3;
        TToolButton *ToolButton1;
        TToolButton *ToolButton2;
        TImageList *ImageList1;
        TSpeedButton *SpeedButton1;
        TToolButton *ToolButton3;
        TSpeedButton *CZoomIn;
        TSpeedButton *CZoomOut;
        TToolButton *ToolButton4;
        TToolButton *ToolButton5;
        TPanel *Panel1;
        TLabel *Label4;
        TCSpinEdit *CEditNumEvn;
        TPanel *Panel3;
        TLabeledEdit *CEditTitleWork;
        TCSpinEdit *CEditTWork;
        TLabel *Label2;
        TCSpinEdit *CEditStartWork;
        TLabel *Label1;
        TLabel *Label3;
        TCSpinEdit *CEditEndWork;
        TSpeedButton *SpeedButton2;
        TPanel *Panel4;
        TCheckBox *CheckBox1;
        TCheckBox *CheckBox2;
        TCheckBox *CheckBox3;
        TEdit *Edit1;
        TToolButton *ToolButton6;
        void __fastcall FormCreate(TObject *Sender);
        void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
        void __fastcall CSpinEdit1Change(TObject *Sender);
        void __fastcall SpeedButton1Click(TObject *Sender);
        void __fastcall COnOffGridClick(TObject *Sender);        
        void __fastcall ImageGrafMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
        void __fastcall ImageGrafMouseMove(TObject *Sender,
          TShiftState Shift, int X, int Y);
        void __fastcall ImageGrafMouseUp(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
        void __fastcall ImageGrafPaint(TObject *Sender);
        void __fastcall CSpinEdit1KeyPress(TObject *Sender, char &Key);
        void __fastcall CModeSelectClick(TObject *Sender);
        void __fastcall CModeAddEvnClick(TObject *Sender);
        void __fastcall CmodeAddWorkClick(TObject *Sender);
        void __fastcall CDelClick(TObject *Sender);
        void __fastcall CEditTitleWorkChange(TObject *Sender);
        void __fastcall CEditTWorkChange(TObject *Sender);
        void __fastcall CEditStartWorkChange(TObject *Sender);
        void __fastcall CEditEndWorkChange(TObject *Sender);
        void __fastcall CEditNumEvnChange(TObject *Sender);
        void __fastcall CLoadClick(TObject *Sender);
        void __fastcall CSaveClick(TObject *Sender);
        void __fastcall CSaveImageClick(TObject *Sender);
        void __fastcall CCalcClick(TObject *Sender);
        void __fastcall CClearClick(TObject *Sender);
        void __fastcall CEditTWorkKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
        void __fastcall CEditStartWorkKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
        void __fastcall CEditEndWorkKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
        void __fastcall CEditNumEvnKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
        void __fastcall ScrollBox1Resize(TObject *Sender);
        void __fastcall CZoomInClick(TObject *Sender);
        void __fastcall CZoomOutClick(TObject *Sender);
        void __fastcall SpeedButton2Click(TObject *Sender);
private:	// User declarations
        Variant App,Sh;
        void OutPrmtWork(const int);
        void OutPrmtEvent(const int);
public:		// User declarations
        __fastcall TForm1(TComponent* Owner);
          
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------

#endif
